//
// Created by 陶源 on 15/9/29.
//

#ifndef FSIO_HELPERS_H
#define FSIO_HELPERS_H

#include <vector>
#include <v8.h>
#include <nan.h>
#include <errno.h>
#include <node_buffer.h>

using namespace v8;
using namespace node;

#define DEBUG

#define V8STR(str) Nan::New<String>(str).ToLocalChecked()
#define V8SYM(str) Nan::New<String>(str).ToLocalChecked()

#define THROW_BAD_ARGS(FAIL_MSG) return Nan::ThrowTypeError(FAIL_MSG);
#define THROW_ERROR(FAIL_MSG) return Nan::ThrowError(FAIL_MSG);
#define CHECK_N_ARGS(MIN_ARGS) if (info.Length() < MIN_ARGS) { THROW_BAD_ARGS("Expected " #MIN_ARGS " arguments") }

const PropertyAttribute CONST_PROP = static_cast<PropertyAttribute>(ReadOnly|DontDelete);

inline static void setConst(Handle<Object> obj, const char* const name, Handle<Value> value){
  obj->ForceSet(Nan::New<String>(name).ToLocalChecked(), value, CONST_PROP);
}

#define ENTER_CONSTRUCTOR(MIN_ARGS) \
	Nan::HandleScope scope;              \
	if (!info.IsConstructCall()) return Nan::ThrowError("Must be called with `new`!"); \
	CHECK_N_ARGS(MIN_ARGS);

#define ENTER_CONSTRUCTOR_POINTER(CLASS, MIN_ARGS) \
	ENTER_CONSTRUCTOR(MIN_ARGS)                    \
	if (!info.Length() || !info[0]->IsExternal()){ \
		return Nan::ThrowError("This type cannot be created directly!"); \
	}                                               \
	auto that = static_cast<CLASS*>(External::Cast(*info[0])->Value()); \
	that->attach(info.This())

#define ENTER_METHOD(CLASS, MIN_ARGS) \
	Nan::HandleScope scope;                \
	CHECK_N_ARGS(MIN_ARGS);           \
	auto that = Nan::ObjectWrap::Unwrap<CLASS>(info.This()); \
	if (that == NULL) { THROW_BAD_ARGS(#CLASS " method called on invalid object") }

#define ENTER_ACCESSOR(CLASS) \
		Nan::HandleScope scope;                \
		auto that = Nan::ObjectWrap::Unwrap<CLASS>(info.Holder());

#define UNWRAP_ARG(CLASS, NAME, ARGNO)     \
	if (!info[ARGNO]->IsObject())          \
		THROW_BAD_ARGS("Parameter " #NAME " is not an object"); \
	auto NAME = Nan::ObjectWrap::Unwrap<CLASS>(Handle<Object>::Cast(info[ARGNO])); \
	if (!NAME)                             \
		THROW_BAD_ARGS("Parameter " #NAME " (" #ARGNO ") is of incorrect type");

#define STRING_ARG(NAME, N) \
	if (info.Length() > N){ \
		if (!info[N]->IsString()) \
			THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be string"); \
		NAME = *String::Utf8Value(info[N]->ToString()); \
	}

#define DOUBLE_ARG(NAME, N) \
	if (!info[N]->IsNumber()) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be number"); \
	NAME = info[N]->ToNumber()->Value();

#define INT_ARG(NAME, N) \
	if (!info[N]->IsNumber()) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be number"); \
	NAME = info[N]->ToInt32()->Value();

#define BOOL_ARG(NAME, N) \
	NAME = false;    \
	if (info.Length() > N){ \
		if (!info[N]->IsBoolean()) \
			THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be bool"); \
		NAME = info[N]->ToBoolean()->Value(); \
	}

#define BUFFER_ARG(NAME, N) \
	if (!Buffer::HasInstance(info[N])) \
		THROW_BAD_ARGS("Parameter " #NAME " (" #N ") should be Buffer object"); \
	NAME = info[N]->ToObject();

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
	Local<Function> callback; \
	if (info.Length() > (CALLBACK_ARG_IDX)) { \
		if (!info[CALLBACK_ARG_IDX]->IsFunction()) { \
			 /* return Nan::ThrowTypeError("Argument " #CALLBACK_ARG_IDX " must be a function"); */ \
		} else { \
      callback = Local<Function>::Cast(info[CALLBACK_ARG_IDX]); \
    } \
	}

#define NAN_CALLBACK_ARG(CALLBACK_ARG_IDX) \
	Nan::Callback *callback = NULL; \
	if (info.Length() > (CALLBACK_ARG_IDX)) { \
		if (!info[CALLBACK_ARG_IDX]->IsFunction()) { \
			 /* return Nan::ThrowTypeError("Argument " #CALLBACK_ARG_IDX " must be a function"); */ \
		} else { \
      callback = new Nan::Callback(info[CALLBACK_ARG_IDX].As<Function>()); \
    } \
	}

#ifdef DEBUG
#define DEBUG_HEADER fprintf(stderr, "fsio [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__);
#define DEBUG_FOOTER fprintf(stderr, "\n");
#define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
#define DEBUG_LOG(...)
#endif

inline static Local<Value> CREATE_ERRNO_ERROR(int errorno) {
  const char* err = strerror(errorno);
  Local<Value> e  = Nan::Error(err);
  e->ToObject()->Set(Nan::New("errno").ToLocalChecked(), Nan::New(errorno));
  e->ToObject()->Set(Nan::New("code").ToLocalChecked(), Nan::New(errorno));
  return e;
}

#endif //FSIO_HELPERS_H
