// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "JsonObjectConverter.h"
#include "WebSocketsModule.h"
#include <condition_variable>
#include <mutex>

#include "WebSocketStructs.h"

struct FWebSocketConfiguration
{
	/**
	 * Default config values
	 */
	int32 Num_Retries = 10;

	int32 Sleep_Length = 3;
};

struct FWebSocketAsyncAwaitResponse
{
	bool IsConnected = false;
};

class WEBSOCKETTEST_API FWebSocketClient
{
	public:
	struct FWebSocketConfiguration Configuration;
	bool Connected = false;
	FWebSocketsModule* WebSocketModule;
	IWebSocket* WebSocket;

	explicit FWebSocketClient(struct FWebSocketConfiguration);

	FWebSocketClient();

	void SetNumRetries(int32);

	void SetSleepLength(int32);

	void ConnectToServer();

	void DisconnectFromServer() const;

	bool IsConnected() const;


	// Prevents the game from crashing if the client is in the middle of trying to reconnect
	void Quit();

	template <typename TRequest, typename TRequestData>
	TSharedPtr<FJsonObject> CreateWebSocketRequest(TRequestData Data, const bool AckRequired)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("id", ++Counter > 0 ? Counter : 1);
		JsonObject->SetNumberField("ack", AckRequired ? 1 : 0);
		JsonObject->SetStringField("msgType", Data.GetName());
		JsonObject->SetObjectField("data", FJsonObjectConverter::UStructToJsonObject<TRequestData>(Data));
		return JsonObject;
	}

	template <typename TRequest, typename TRequestData, typename TResponseData>
	TResponseData SendAsync(TRequestData RequestData, const bool AckRequired)
	{
		FString JsonRequest;
		auto WebSocketRequest = CreateWebSocketRequest<TRequest, TRequestData>(RequestData, AckRequired);

		if (AckRequired)
		{
			// FWebSocketResponse Response {nullptr, false};
			AckMap.Add(WebSocketRequest->GetNumberField("id"), nullptr);
		}
		const TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(WebSocketRequest.ToSharedRef(), Writer);

		UE_LOG(LogTemp, Log, TEXT("%s"), *JsonRequest);

		WebSocket->Send(JsonRequest);

		UE_LOG(LogTemp, Log, TEXT("Request sent"));

		if (!AckRequired) return {};

		TFunction<TResponseData()> Task = [this, &WebSocketRequest] {

			UE_LOG(LogTemp, Log, TEXT("Before WaitForResponseAsync"));
			return WaitForResponseAsync<TResponseData>(WebSocketRequest->GetNumberField("id"));
		};

		UE_LOG(LogTemp, Log, TEXT("Before Async"));

		auto FutureResponse = Async(EAsyncExecution::TaskGraph, Task);

		UE_LOG(LogTemp, Log, TEXT("After Async"));

		UE_LOG(LogTemp, Log, TEXT("Before Response.Wait"));

		FutureResponse.Wait();

		UE_LOG(LogTemp, Log, TEXT("After Response.Wait"));

		return FutureResponse.Get();
	};

	/**
	* Delegate called when websocket connection closed wilfully.
	*/
	DECLARE_EVENT(FWebSocketClient, FClosedEvent);
	FClosedEvent OnClosed;

	/**
	* Delegate called when websocket attempts to reconnect to the server.
	*/
	DECLARE_EVENT(FWebSocketClient, FReconnectionEvent);
	FReconnectionEvent OnReconnection;

	TMulticastDelegate<void(bool)> ConnectionDelegate;

	private:
	TMap<int, TSharedPtr<FJsonObject>> AckMap;
	int32 Retries = 0;
	uint64  Counter = 0;
	bool IsReconnecting = false;
	bool QuittingFlag = false;

	std::mutex Mutex;
	std::condition_variable RequestCV, ReconnectingCV, QuittingCV;

	FWebSocketAsyncAwaitResponse AsyncAwaitResponse;

	void Reconnect();

	void ProcessResponse(FString);

	void BindResponseDelegate(const bool);


	template <typename TResponseData>
	TResponseData WaitForResponseAsync(const int32 Id)
	{
		UE_LOG(LogTemp, Log, TEXT("Inside WaitForResponseAsync"));
		TResponseData Data;

		UE_LOG(LogTemp, Log, TEXT("Waiting for notify"));

		std::unique_lock<std::mutex> UniqueLock(Mutex);
		UE_LOG(LogTemp, Log, TEXT("Inside lock"));
		RequestCV.wait(UniqueLock);
		UE_LOG(LogTemp, Log, TEXT("After lock wait"));
		Mutex.unlock();

		UE_LOG(LogTemp, Log, TEXT("After wait"));

		FJsonObjectConverter::JsonObjectToUStruct(AckMap[Id].ToSharedRef(), &Data);

		return Data;
	}
};