#include "WebSocketClient.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"

FWebSocketClient::FWebSocketClient(): WebSocket(nullptr)
{
	const FWebSocketConfiguration Config = FWebSocketConfiguration();
	WebSocketModule = &FWebSocketsModule::Get();
	Configuration.Num_Retries = Config.Num_Retries;
	Configuration.Sleep_Length = Config.Sleep_Length;
}

FWebSocketClient::FWebSocketClient(const struct FWebSocketConfiguration Config): WebSocket(nullptr)
{
	WebSocketModule = &FWebSocketsModule::Get();
	Configuration.Num_Retries = Config.Num_Retries;
	Configuration.Sleep_Length = Config.Sleep_Length;
}


void FWebSocketClient::SetNumRetries(const int32 N)
{
	Configuration.Num_Retries = N;
}

void FWebSocketClient::SetSleepLength(const int32 N)
{
	Configuration.Sleep_Length = N;
}

//Connects to the server
void FWebSocketClient::ConnectToServer()
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
	{
		const TSharedRef<IWebSocket> IWebSocket = WebSocketModule->CreateWebSocket("ws://localhost:5000/ws");
		WebSocket = &IWebSocket.Get();

		ConnectionDelegate.AddRaw(this, &FWebSocketClient::BindResponseDelegate);

		WebSocket->OnConnected().AddLambda([this]() {
			UE_LOG(LogTemp, Log, TEXT("Connected to websocket server."));
			ConnectionDelegate.Broadcast(true);
			Connected = true;
		});

		WebSocket->OnConnectionError().AddLambda([this](const FString& Error) {
			UE_LOG(LogTemp, Log, TEXT("Failed to connect to websocket server with error: \"%s\"."), *Error);
			ConnectionDelegate.Broadcast(false);
			Connected = false;
		});

		WebSocket->OnMessage().AddLambda([this](const FString& Message) {
			UE_LOG(LogTemp, Log, TEXT("Received message from websocket server: \"%s\"."), *Message);
			ProcessResponse(Message);
		});

		WebSocket->OnClosed().AddLambda([this](const int32 StatusCode, const FString& Reason, bool bWasClean) {
			UE_LOG(LogTemp, Log, TEXT("Connection to websocket server has been closed with status code: \"%d\" and reason: \"%s\"."), StatusCode, *Reason);
			Connected = false;
			if (StatusCode == 1000)
			{
				OnClosed.Broadcast();
			} else
			{
				UE_LOG(LogTemp, Log, TEXT("Attempting to reconnect..."));
				IsReconnecting = true;
				OnReconnection.Broadcast();
				Retries = Configuration.Num_Retries;
				Reconnect();
			}
		});

		std::unique_lock<std::mutex> UniqueLock(Mutex);
		WebSocket->Connect();
		ReconnectingCV.wait(UniqueLock);
		Mutex.unlock();
	});
}

// Disconnect from the server
void FWebSocketClient::DisconnectFromServer() const
{
	WebSocket->Close();
}

// Reconnect to the server
void FWebSocketClient::Reconnect()
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
	{
		UE_LOG(LogTemp, Log, TEXT("Retries left: \"%d\""), Retries);

		if (!QuittingFlag)
		{
			if (Retries > 0)
			{
				--Retries;

				std::unique_lock<std::mutex> UniqueLock(Mutex);
				ConnectToServer();
				ReconnectingCV.wait(UniqueLock);
				FPlatformProcess::Sleep(Configuration.Sleep_Length);
				Mutex.unlock();

				if (!Connected && Retries == 0)
				{
					AsyncTask(ENamedThreads::GameThread, [this]()
					{
						UE_LOG(LogTemp, Log, TEXT("Client timed-out"));
						IsReconnecting = false;
						OnClosed.Broadcast();
					});
				} else if (Connected)
				{
					UE_LOG(LogTemp, Log, TEXT("Successful Reconnection"));
					IsReconnecting = false;
				} else
				{
					Reconnect();
				}
			}
		} else
		{
			QuittingCV.notify_one();
		}
	});
}

void FWebSocketClient::ProcessResponse(const FString Message)
{
	TSharedPtr<FJsonObject> JsonResponse;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Message);

	if (FJsonSerializer::Deserialize(JsonReader, JsonResponse))
	{
		//TODO: if id is 0, need to add it to a push queue, otherwise, all pushes will overwrite each other
		const int32 Id = JsonResponse->GetIntegerField("id");
		if (Id == 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Enqueuing Push: %s"), *JsonResponse->GetStringField("event"));
			PushMessageQueue.Enqueue(JsonResponse);
			return;
		}

		WriteAckMap(Id, JsonResponse);

		RequestCV.notify_one();

		UE_LOG(LogTemp, Log, TEXT("After JsonObject set to AckMap[Id] and notify RequestCV"));
	} else
	{
		UE_LOG(LogTemp, Log, TEXT("Couldn't deserialize"));
	}
}

void FWebSocketClient::ProcessPushMessages(uint32 MaxMessages)
{
	TSharedPtr<FJsonObject> JsonResponse;
	while (MaxMessages-- > 0 && PushMessageQueue.Dequeue(JsonResponse))
	{
		auto EventName = JsonResponse->GetStringField("event");
		if (!TypeRegistry.Contains(EventName))
		{
			UE_LOG(LogTemp, Log, TEXT("Dequeued Unknown Json Push: %s"), *EventName);
			return;
		}
		const auto PushHandler = TypeRegistry[JsonResponse->GetStringField("event")];
		UE_LOG(LogTemp, Log, TEXT("Dequeued Json: %s"), *EventName);
		PushHandler(JsonResponse);
	}
}

void FWebSocketClient::BindResponseDelegate(const bool IsConnected)
{
	AsyncAwaitResponse.IsConnected = IsConnected;
	ReconnectingCV.notify_one();
}

bool FWebSocketClient::IsConnected() const
{
	return Connected;
}

void FWebSocketClient::Quit()
{
	if (IsReconnecting)
	{
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			std::unique_lock<std::mutex> UniqueLock(Mutex);
			QuittingFlag = true;
			QuittingCV.wait(UniqueLock);
			Mutex.unlock();
		});
	}
}
