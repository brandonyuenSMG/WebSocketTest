// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "JsonObjectConverter.h"
#include "WebSocketsModule.h"
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#include "WebSocketStructs.h"

typedef void (*Func_T)(TSharedPtr<FJsonObject>);
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
	template <typename TRequest>
	TSharedPtr<FJsonObject> CreateWebSocketRequest(const TRequest& Data, const bool AckRequired)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("id", ++Counter > 0 ? Counter : 1);
		JsonObject->SetNumberField("ack", AckRequired ? 1 : 0);
		JsonObject->SetStringField("msgType", Data.GetName());
		JsonObject->SetObjectField("data", FJsonObjectConverter::UStructToJsonObject(Data));
		return JsonObject;
	}

	template <typename TRequest, typename TResponseData>
	TResponseData SendAsync(const TRequest& RequestData, const bool AckRequired = true, uint TimeoutMs = 5000)
	{
		FString JsonRequest;
		auto WebSocketRequest = CreateWebSocketRequest(RequestData, AckRequired);

		if (AckRequired)
		{
			// FWebSocketResponse Response {nullptr, false};
			WriteAckMap(WebSocketRequest->GetNumberField("id"), nullptr);
		}
		const auto Writer = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(WebSocketRequest.ToSharedRef(), Writer);

		UE_LOG(LogTemp, Log, TEXT("%s"), *JsonRequest);

		WebSocket->Send(JsonRequest);

		UE_LOG(LogTemp, Log, TEXT("Request sent"));

		if (!AckRequired) return {};

		UE_LOG(LogTemp, Log, TEXT("Before Async"));

		bool Success = false;
		//If there is an error event, it will be returned in the Pair.Value'
		//otherwise, the response will be in Pair.Key
		auto FutureResponse = Async(EAsyncExecution::TaskGraph, [this, WebSocketRequest, &Success, TimeoutMs] {
			UE_LOG(LogTemp, Log, TEXT("Before WaitForResponseAsync"));
			TPair<TResponseData, FMgsError> Pair;
			try
			{
				Pair.Key = WaitForResponseAsync<TResponseData>(WebSocketRequest->GetNumberField("id"), TimeoutMs);
				Success = true;
			} catch (const FMgsError& e)
			{
				Pair.Value = e;
			}
			return Pair;
		});

		UE_LOG(LogTemp, Log, TEXT("After Async"));

		UE_LOG(LogTemp, Log, TEXT("Before Response.Wait"));

		FutureResponse.Wait();
		UE_LOG(LogTemp, Log, TEXT("After Response.Wait"));

		if (!Success)
		{
			throw FutureResponse.Get().Value;
		}
		return FutureResponse.Get().Key;
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

	template <typename T>
	void On(FString EventName, std::function<T*()> Handler)
	{
		TypeRegistry.Add(EventName, [Handler](TSharedPtr<FJsonObject> JsonObject)
		{
			T PushedMessage;
			FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &PushedMessage);
			UE_LOG(LogTemp, Log, TEXT("Got pushed message"));
			Handler(PushedMessage);
		});
	}

	private:
	TMap<int, TSharedPtr<FJsonObject>> AckMap;
	TQueue<TSharedPtr<FJsonObject>> PushMessageQueue;
	TMap<FString, Func_T> TypeRegistry;
	int32 Retries = 0;
	uint64  Counter = 0;
	bool IsReconnecting = false;
	bool QuittingFlag = false;
	std::shared_timed_mutex AckMutex;
	std::mutex Mutex;
	std::condition_variable RequestCV, ReconnectingCV, QuittingCV;

	FWebSocketAsyncAwaitResponse AsyncAwaitResponse;

	void Reconnect();

	void ProcessResponse(FString);
	void ProcessPushMessages(uint32 MaxMessages);

	void BindResponseDelegate(const bool);

	TSharedPtr<FJsonObject> ReadAckMap(const int32 Id)
	{
		std::shared_lock<std::shared_timed_mutex> Lock(AckMutex);
		return AckMap[Id];
	}

	void WriteAckMap(const int32 Id, const TSharedPtr<FJsonObject> JsonObject)
	{
		std::unique_lock<std::shared_timed_mutex> Lock(AckMutex);
		AckMap.Add(Id, JsonObject);
	}

	void RemoveFromAckMap(const int32 Id)
	{
		std::unique_lock<std::shared_timed_mutex> Lock(AckMutex);
		AckMap.Remove(Id);
	}

	TSharedPtr<FJsonObject> WaitForAck(const int32 Id, const uint TimeoutMs = 5000, const uint PollIntervalMs = 10)
	{
		uint Elapsed = 0;
		const auto SleepDuration = PollIntervalMs*1ms;
		while (true)
		{
			const auto Ack = ReadAckMap(Id);
			if (Ack != nullptr) return Ack;
			if (Ack == nullptr)
			{
				Elapsed += PollIntervalMs;
				if (Elapsed >= TimeoutMs) return nullptr;
				std::this_thread::sleep_for(SleepDuration);
			}
		}
	}

	template <typename TResponseData>
	TResponseData WaitForResponseAsync(const int32 Id, const uint TimeoutMs = 5000)
	{
		UE_LOG(LogTemp, Log, TEXT("Inside WaitForResponseAsync"));
		TResponseData Data;

		UE_LOG(LogTemp, Log, TEXT("Waiting for notify"));

		std::unique_lock<std::mutex> UniqueLock(Mutex);
		const auto Ack = WaitForAck(Id, TimeoutMs);
		RemoveFromAckMap(Id);
		if (Ack == nullptr)
		{
			UE_LOG(LogTemp, Log, TEXT("Request timeout"));
			FMgsError Error;
			Error.Message = "Timeout";
			throw Error;
		}
		UE_LOG(LogTemp, Log, TEXT("Got Ack"));

		const auto NestedJsonData = Ack->GetObjectField("data");
		UE_LOG(LogTemp, Log, TEXT("Got Event: %s"), *Ack->GetStringField("event"));
		if (Ack->GetStringField("event").Compare("Error") == 0)
		{
			FMgsError Error;
			FJsonObjectConverter::JsonObjectToUStruct(NestedJsonData.ToSharedRef(), &Error);
			UE_LOG(LogTemp, Log, TEXT("Got mgs error response"));
			throw Error;
		}
		FJsonObjectConverter::JsonObjectToUStruct(NestedJsonData.ToSharedRef(), &Data);
		UE_LOG(LogTemp, Log, TEXT("Got response"));
		return Data;
	}
};