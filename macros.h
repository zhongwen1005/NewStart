#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>

typedef struct
{
    unsigned int id;
    const char* desc;
}ENUM_STR_TYPE;

#define ENUM_STR_BEGIN(type) static ENUM_STR_TYPE  ENUM_DESC__##type[]={ 
#define ENUM_STR_END  {0xffff,"end"} };
#define ADD_ENUM_STR_DESC(id,str) {id, #str},
#define GET_ENUM_STR(type,id) get_desc_by_id(ENUM_DESC__##type,id)

const char* get_desc_by_id(ENUM_STR_TYPE* desc, unsigned int id)
{
    int i = 0;
    static char no_match[10] = { 0 };
    while (desc[i].id != 0xffff) {
        if (desc[i].id == id)
            return desc[i].desc;
        i++;
    }

    return nullptr;
}

template <typename EnumType>
struct SEnumName {
    static const char * List[];
};

template<typename EnumType>
EnumType ConvertStringToEnum(const char* pStr)
{
    EnumType obj = static_cast<EnumType>(-1);
    int count = sizeof(SEnumName<EnumType>::List) /
        sizeof(SEnumName<EnumType>::List[0]);
    for (int i = 0; i < count; ++i)
    {
        const char* ch = SEnumName<EnumType>::List[i];
        if (!abs(strcmp(pStr, SEnumName<EnumType>::List[i])))
        {
            obj = static_cast<EnumType>(i);
            break;
        }
    }
    return obj;
}

#define ENUM_INT_BEGIN(type) const char * SEnumName<##type>::List[]={ 
#define ENUM_INT_END  {"end"} };
#define ADD_ENUM_INT_DESC(str) {#str},

typedef enum ExchangeEnum {
    S = 0,
    B = 1
}ExchangeEnum;

ENUM_STR_BEGIN(ExchangeEnum)
ADD_ENUM_STR_DESC(ExchangeEnum::S, S)
ADD_ENUM_STR_DESC(ExchangeEnum::B, B)
ENUM_STR_END

ENUM_INT_BEGIN(ExchangeEnum)
ADD_ENUM_INT_DESC(S)
ADD_ENUM_INT_DESC(B)
ENUM_INT_END

typedef enum OrderEnum {
    A = 0,
    X = 1,
    M = 2
}OrderEnum;

ENUM_STR_BEGIN(OrderEnum)
ADD_ENUM_STR_DESC(OrderEnum::A, A)
ADD_ENUM_STR_DESC(OrderEnum::X, X)
ADD_ENUM_STR_DESC(OrderEnum::M, M)
ENUM_STR_END

ENUM_INT_BEGIN(OrderEnum)
ADD_ENUM_INT_DESC(A)
ADD_ENUM_INT_DESC(X)
ADD_ENUM_INT_DESC(M)
ENUM_INT_END