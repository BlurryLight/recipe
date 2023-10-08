// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "libplatform/libplatform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

#include "v8.h"

int main(int argc, char *argv[]) {
  // Initialize V8.
  v8::StartupData SnapshotBlob;
  SnapshotBlob.data = (const char *)SnapshotBlobCode;
  SnapshotBlob.raw_size = sizeof(SnapshotBlobCode);
  v8::V8::SetSnapshotDataBlob(&SnapshotBlob);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate *isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::ObjectTemplate> globalObject =
        v8::ObjectTemplate::New(isolate);
    v8::Local<v8::Context> context =
        v8::Context::New(isolate, nullptr, globalObject);
    v8::Context::Scope context_scope(context);
    // init js
    v8::TryCatch TryCatch(isolate);
    {
      const char csource[] = R"(
         var foo = function(...args) { 
            let s = 'foo called from cpp';
            for (const item of args) {
                s += String(item) + ' ';
            }
            return s;
        }
      )";
      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source =
          v8::String::NewFromUtf8Literal(isolate, csource);
      // Compile the source code.
      v8::Local<v8::Script> script =
          v8::Script::Compile(context, source).ToLocalChecked();
      // Run the script to get the result.
      v8::Local<v8::Value> Result;
      bool bValid = script->Run(context).ToLocal(&Result);
      if (!bValid) {
        v8::String::Utf8Value e(isolate, TryCatch.Exception());
        printf("Exception: %s\n", *e);
      }
    }
    // get object
    {
      v8::Local<v8::Function> fooFunc =
          context->Global()
              ->Get(context, v8::String::NewFromUtf8Literal(isolate, "foo"))
              .ToLocalChecked()
              .As<v8::Function>();
      std::vector<v8::Local<v8::Value>> args;
      for(int i =0 ; i < 10; i++)
      {
        args.push_back(v8::Number::New(isolate, i));
      }
      v8::Local<v8::Value> Ret =
          fooFunc
              ->CallAsFunction(context, context->Global(), (uint32_t)args.size(),
                               args.data())
              .ToLocalChecked();
      v8::String::Utf8Value RetStr(isolate, Ret);
      printf("foo return: %s\n", *RetStr);
    }
  }
  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete create_params.array_buffer_allocator;
  return 0;
}