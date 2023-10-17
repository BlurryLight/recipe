// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <string>
#include <limits>

#include "Binding.hpp"
#include "CppObjectMapper.h"

#include "libplatform/libplatform.h"
#include "v8.h"

#if defined(PLATFORM_WINDOWS)

#if _WIN64
#include "Blob/Win64/SnapshotBlob.h"
#else
#include "Blob/Win32/SnapshotBlob.h"
#endif

#elif defined(PLATFORM_ANDROID_ARM)
#include "Blob/Android/armv7a/SnapshotBlob.h"
#elif defined(PLATFORM_ANDROID_ARM64)
#include "Blob/Android/arm64/SnapshotBlob.h"
#elif defined(PLATFORM_MAC)
#include "Blob/macOS/SnapshotBlob.h"
#elif defined(PLATFORM_IOS)
#include "Blob/iOS/arm64/SnapshotBlob.h"
#elif defined(PLATFORM_LINUX)
#include "Blob/Linux/SnapshotBlob.h"
#endif

class TestClass
{
public:
    TestClass(int p) {
        std::cout << "TestClass(" << p << ")" << std::endl;
        X = p;
        Arr[0] = 100;
        Arr[1] = 101;
        Arr[2] = 102;
        Y = UINT_MAX;
        ptr = Arr;
    }

    static void Print(std::string msg) {
        std::cout << msg << std::endl;
    }
    
    int Add(int a, int b)
    {
        std::cout << "Add(" << a << "," << b << ")" << std::endl;
        return a + b;
    }

    void PointerAdd(uint32_t* pt)
    {
        if(pt)
        {
            *pt = (*pt) + 1;
        }
    }

    int X;
    uint32_t Arr[3];
    uint32_t Y;
    uint32_t* ptr;
};

UsingCppType(TestClass);

struct Point1f {
    float x = 0;
};

void GetPointX(v8::Local<v8::String> property,
               const v8::PropertyCallbackInfo<v8::Value> &Info) {
    v8::Local<v8::Object> self = Info.Holder();
    if (self->IsObject()) {
        Point1f *p =
            (Point1f *)self->GetInternalField(0).As<v8::External>()->Value();
        Info.GetReturnValue().Set(p->x);
    }
}

void SetPointX(v8::Local<v8::String> property, v8::Local<v8::Value> value,
               const v8::PropertyCallbackInfo<void> &Info) {
    v8::Local<v8::Object> self = Info.Holder();
    v8::Local<v8::Context> context = Info.GetIsolate()->GetCurrentContext();
    if (self->IsObject()) {
        Point1f *p =
            (Point1f *)self->GetInternalField(0).As<v8::External>()->Value();
        p->x = (float)(value->NumberValue(context).FromMaybe(0.0));
    }
}


int main(int argc, char *argv[]) {
    // Initialize V8.
    v8::StartupData SnapshotBlob;
    SnapshotBlob.data = (const char *)SnapshotBlobCode;
    SnapshotBlob.raw_size = sizeof(SnapshotBlobCode);
    v8::V8::SetSnapshotDataBlob(&SnapshotBlob);

    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    
    puerts::FCppObjectMapper cppObjectMapper;
    {
        v8::Isolate::Scope isolate_scope(isolate);

        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(isolate);

        v8::Local<v8::ObjectTemplate> Point1fTemplate = v8::ObjectTemplate::New(isolate);
        Point1fTemplate->SetInternalFieldCount(1); // for one pointer
        Point1fTemplate->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "x"),
                                    GetPointX, SetPointX);

        v8::Local<v8::FunctionTemplate> dummyFt =
            v8::FunctionTemplate::New(isolate, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                if(info.IsConstructCall())
                {
                    std::cout << "construct dummy object" << std::endl;
                }
            });
        dummyFt->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Dummy"));
        v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        global->Set(v8::String::NewFromUtf8Literal(isolate, "Dummy"), dummyFt);
        dummyFt->InstanceTemplate()->Set(
            v8::String::NewFromUtf8Literal(isolate, "InstanceProperty"),
            v8::Number::New(isolate, 1));

        dummyFt->PrototypeTemplate()->Set(
            v8::String::NewFromUtf8Literal(isolate, "ProtoProperty"),
            v8::Number::New(isolate, 2));

        // Create a new context.
        v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, global);

        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);
        
        cppObjectMapper.Initialize(isolate, context);
        isolate->SetData(MAPPER_ISOLATE_DATA_POS, static_cast<puerts::ICppObjectMapper*>(&cppObjectMapper));

        // 这下面一块是
        // 1. 向JS里注册一个loadCppType函数
        // 2. 这个函数调用的时候会回到loadcpptypefunc, 又转发到  FCppObjectMapper->LoadCppType
        // 里面应该是返回了一个带有prototype的funciton之类的东西，所以js层就可以new了
        auto LoadCppTypeFunc =
            [](const v8::FunctionCallbackInfo<v8::Value> &info) {
              auto pom = static_cast<puerts::FCppObjectMapper *>(
                  (v8::Local<v8::External>::Cast(info.Data()))->Value());
              pom->LoadCppType(info);
            };
        //  创建FUnctiontemplate的时候可以传入一个data，这个data会在每次调用的时候传回来
        auto LoadCppTypeFuncTemplate = v8::FunctionTemplate::New(
            isolate, LoadCppTypeFunc,
            v8::External::New(isolate, &cppObjectMapper));

        auto LoadCppTypeFuncObject =
            LoadCppTypeFuncTemplate->GetFunction(context).ToLocalChecked();
        context->Global()
            ->Set(context,
                  v8::String::NewFromUtf8(isolate, "loadCppType")
                      .ToLocalChecked(),
                  LoadCppTypeFuncObject)
            .Check();

        // Point1f* p = new Point1f();
        // auto Point1fObj = Point1fTemplate->NewInstance(context).ToLocalChecked();
        // Point1fObj->SetInternalField(0, v8::External::New(isolate, p));

        // (void)context->Global()->Set(
        //     context, v8::String::NewFromUtf8(isolate, "Point1fObj").ToLocalChecked(),
        //     Point1fObj);

        //注册
        puerts::DefineClass<TestClass>()
            .Constructor<int>()
            .Function("Print", MakeFunction(&TestClass::Print))
            .Property("X", MakeProperty(&TestClass::X))
            .Property("Y", MakeProperty(&TestClass::Y))
            .Property("Ptr", MakeProperty(&TestClass::ptr))
            .Property("Arr", MakeReadonlyProperty(&TestClass::Arr))
            .Method("Add", MakeFunction(&TestClass::Add))
            .Method("PointerAdd", MakeFunction(&TestClass::PointerAdd))
            .Register();

        {
            const char* csource = R"(
                const TestClass = loadCppType('TestClass');
                TestClass.Print('hello world');
                let obj = new TestClass(123);
                TestClass.Print(obj.constructor.name);
                
                TestClass.Print(obj.X);
                obj.X = 99;
                TestClass.Print(obj.X);
                TestClass.Print(obj.Y);
                TestClass.Print(obj.Arr);
                obj.PointerAdd(obj.Ptr);
                const typedArray = new Uint32Array(obj.Arr);
                TestClass.Print([...typedArray]);

                // TestClass.Print('ret = ' + obj.Add(1, 3));

                // let dummy = new Dummy();
                // TestClass.Print(JSON.stringify(dummy));
                // TestClass.Print(dummy.InstanceProperty);
                // TestClass.Print(dummy.constructor);
                // var prototype = Object.getPrototypeOf(dummy);
                // TestClass.Print(JSON.stringify(prototype));
                // TestClass.Print(dummy.hasOwnProperty('InstanceProperty'));
                // TestClass.Print(dummy.hasOwnProperty('ProtoProperty'));
                // TestClass.Print('Point1fObj.x = ' + Point1fObj.x);
                // Point1fObj.x = 100;
                // TestClass.Print('Point1fObj.x = ' + Point1fObj.x);
              )";

            // Create a string containing the JavaScript source code.
            v8::Local<v8::String> source =
                v8::String::NewFromUtf8(isolate, csource)
                .ToLocalChecked();

            // Compile the source code.
            v8::Local<v8::Script> script =
                v8::Script::Compile(context, source).ToLocalChecked();

            // Run the script
            auto _unused = script->Run(context);
        }
        
        cppObjectMapper.UnInitialize(isolate);
    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
