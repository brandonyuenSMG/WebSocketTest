#pragma once

#include "WebSocketStructs.generated.h"



USTRUCT()
struct FWebSocketRequest
{
	GENERATED_BODY()

	UPROPERTY()
	uint64 Id = 0; //unique id, to line up with responses

	UPROPERTY()
	int32 Ack = 0; //set to 1 to indicate to server to send a reply

	UPROPERTY()
	FString MsgType; //endpoint path
};

USTRUCT()
struct  FDebugLoginRequestData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Token;

	FString Name = "DebugLogin";

	FString GetName() const;
};

USTRUCT()
struct FDebugLoginResponseData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Created;

	UPROPERTY()
	FString Updated;
};

USTRUCT()
struct FDebugLogin : public FWebSocketRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FDebugLoginRequestData Data;
};


USTRUCT()
struct FMgsError
{
	GENERATED_BODY()

	UPROPERTY()
	FString Message;

	FString Name = "Error";

	FString GetName() const;
};

USTRUCT()
struct FEchoRequestData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Val;

	FString Name = "Echo";

	FString GetName() const;
};

USTRUCT()
struct FEchoResponseData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Val;
};

USTRUCT()
struct FEcho : public FWebSocketRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FEchoRequestData Data;
};

USTRUCT()
struct FChatMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FString Message;

	UPROPERTY()
	FString SenderId;

};