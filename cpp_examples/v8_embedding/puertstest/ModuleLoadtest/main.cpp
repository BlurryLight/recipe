/*
 * Google V8 JavaScript module importing example
 * by surusek <almostnoruby@live.com>
 * ---
 * based on https://stackoverflow.com/a/52031275,
 * https://chromium.googlesource.com/v8/v8/+/master/samples/hello-world.cc and
 * https://gist.github.com/bellbind/b69c3aa266cffe43940c
 * ---
 * This code snippet is distributed in public domain, so you can do anything
 * you want with it.
 * ---
 * This program takes filename from command line and executes that file as 
 * JavaScript code, loading modules from other files, if nessesary. Since it is
 * only a simple example, all files specified by import are loaded from root
 * directory of the program, e.g. if module "foo/bar.mjs" is imported, and it
 * imports "baz.mjs", it doesn't import "foo/baz.mjs", but "baz.mjs".
 */

/*****************************************************************************
 * Includes
 *****************************************************************************/
// Including V8 libraries
#include <libplatform/libplatform.h>
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
#include <v8.h>

// printf(), exit()
#include <stdio.h>
#include <stdlib.h>

// File operations
#include <fstream>

/*****************************************************************************
 * Declarations
 *****************************************************************************/

// Reads a file to char array; line #140
char* readFile(char filename[]);

// Simple print function binding to JavaScript VM; line #169
void print(const v8::FunctionCallbackInfo<v8::Value>& args);

// Loads a module; line #187
v8::MaybeLocal<v8::Module> loadModule(char code[],
                                      char name[],
                                      v8::Local<v8::Context> cx);

// Check, if module isn't empty (or pointer to it); line #221
v8::Local<v8::Module> checkModule(v8::MaybeLocal<v8::Module> maybeModule,
                                  v8::Local<v8::Context> cx);

// Executes module; line #247
v8::Local<v8::Value> execModule(v8::Local<v8::Module> mod,
                                v8::Local<v8::Context> cx,
                                bool nsObject = false);

// Callback for static import; line #270
v8::MaybeLocal<v8::Module> callResolve(v8::Local<v8::Context> context,
                                       v8::Local<v8::String> specifier,
                                       v8::Local<v8::Module> referrer);

// Callback for dynamic import; line #285
v8::MaybeLocal<v8::Promise> callDynamic(v8::Local<v8::Context> context,
                                        v8::Local<v8::ScriptOrModule> referrer,
                                        v8::Local<v8::String> specifier);

// Callback for module metadata; line #310
void callMeta(v8::Local<v8::Context> context,
              v8::Local<v8::Module> module,
              v8::Local<v8::Object> meta);

/*****************************************************************************
 * int main
 * Application entrypoint.
 *****************************************************************************/
int main(int argc, char* argv[]) {

  // Since program has to parse filename as argument, here is check for that
  if (!(argc == 2)) {
    printf(
        "No argument provided or too much arguments provided! Enter "
        "filename as first one to continue\n");
    exit(EXIT_FAILURE);
  }
  
  // // Where is icudtxx.dat? Does nothing if ICU database is in library itself
  // v8::V8::InitializeICUDefaultLocation(argv[0]);

  // // Where is snapshot_blob.bin? Does nothing if external data is disabled
  // v8::V8::InitializeExternalStartupData(argv[0]);
  v8::StartupData SnapshotBlob;
  SnapshotBlob.data = (const char *)SnapshotBlobCode;
  SnapshotBlob.raw_size = sizeof(SnapshotBlobCode);
  v8::V8::SetSnapshotDataBlob(&SnapshotBlob);

  // Creating platform
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();

  // Initializing V8 VM
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // Creating isolate from the params (VM instance)
  v8::Isolate::CreateParams mCreateParams;
  mCreateParams.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* mIsolate;
  mIsolate = v8::Isolate::New(mCreateParams);

  // Binding dynamic import() callback
  mIsolate->SetHostImportModuleDynamicallyCallback(callDynamic);

  // Binding metadata loader callback
  mIsolate->SetHostInitializeImportMetaObjectCallback(callMeta);
  {

    // Initializing handle scope
    v8::HandleScope handle_scope(mIsolate);

    // Binding print() funtion to the VM; check line #
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(mIsolate);
    global->Set(mIsolate, "print", v8::FunctionTemplate::New(mIsolate, print));

    // Creating context
    v8::Local<v8::Context> mContext = v8::Context::New(mIsolate, nullptr, global);
    v8::Context::Scope context_scope(mContext);
    {

      // Reading a module from file
      char* contents = readFile(argv[1]);

      // Executing module
      v8::Local<v8::Module> mod =
          checkModule(loadModule(contents, argv[1], mContext), mContext);
      v8::Local<v8::Value> returned = execModule(mod, mContext);

      // Returning module value to the user
      v8::String::Utf8Value val(mIsolate, returned);
      printf("Returned value: %s\n", *val);
    }
  }

  // Proper VM deconstructing
  mIsolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete mCreateParams.array_buffer_allocator;
}

/*****************************************************************************
 * char* readFile
 * Reads file contents to a null-terminated string.
 *****************************************************************************/
char* readFile(char filename[]) {

  // Opening file; ifstream::ate is use to determine file size
  std::ifstream file;
  file.open(filename, std::ifstream::ate);
  char* contents;
  if (!file) {
    contents = new char[1];
    return contents;
  }

  // Get file size
  size_t file_size = file.tellg();

  // Return file pointer from end of the file (set by ifstream::ate) to beginning
  file.seekg(0);

  // Reading file to char array and returing it
  std::filebuf* file_buf = file.rdbuf();
  contents = new char[file_size + 1]();
  file_buf->sgetn(contents, file_size);
  file.close();
  return contents;
}

/*****************************************************************************
 * void print
 * Binding of simple console print function to the VM
 *****************************************************************************/
void print(const v8::FunctionCallbackInfo<v8::Value>& args) {

  // Getting arguments; error handling
  v8::Isolate* isolate = args.GetIsolate();
  v8::String::Utf8Value val(isolate, args[0]);
  if (*val == nullptr)
    isolate->ThrowException(
        v8::String::NewFromUtf8(isolate, "First argument of function is empty")
            .ToLocalChecked());

  // Printing
  printf("%s\n", *val);
}

/*****************************************************************************
 * v8::MaybeLocal<v8::Module> loadModule
 * Loads module from code[] without checking it
 *****************************************************************************/
v8::MaybeLocal<v8::Module> loadModule(char code[],
                                      char name[],
                                      v8::Local<v8::Context> cx) {

  // Convert char[] to VM's string type
  v8::Local<v8::String> vcode =
      v8::String::NewFromUtf8(cx->GetIsolate(), code).ToLocalChecked();

  // Create script origin to determine if it is module or not.
  // Only first and last argument matters; other ones are default values.
  // First argument gives script name (useful in error messages), last
  // informs that it is a module.
  v8::ScriptOrigin origin(
      v8::String::NewFromUtf8(cx->GetIsolate(), name).ToLocalChecked(),
      v8::Integer::New(cx->GetIsolate(), 0),
      v8::Integer::New(cx->GetIsolate(), 0), v8::False(cx->GetIsolate()),
      v8::Local<v8::Integer>(), v8::Local<v8::Value>(),
      v8::False(cx->GetIsolate()), v8::False(cx->GetIsolate()),
      v8::True(cx->GetIsolate()));

  // Compiling module from source (code + origin)
  v8::Context::Scope context_scope(cx);
  v8::ScriptCompiler::Source source(vcode, origin);
  v8::MaybeLocal<v8::Module> mod;
  mod = v8::ScriptCompiler::CompileModule(cx->GetIsolate(), &source);

  // Returning non-checked module
  return mod;
}

/*****************************************************************************
 * v8::Local<v8::Module> checkModule
 * Checks out module (if it isn't nullptr/empty)
 *****************************************************************************/
v8::Local<v8::Module> checkModule(v8::MaybeLocal<v8::Module> maybeModule,
                                  v8::Local<v8::Context> cx) {

  // Checking out
  v8::Local<v8::Module> mod;
  if (!maybeModule.ToLocal(&mod)) {
    printf("Error loading module!\n");
    exit(EXIT_FAILURE);
  }

  // Instantianing (including checking out depedencies). It uses callResolve
  // as callback: check #
  v8::Maybe<bool> result = mod->InstantiateModule(cx, callResolve);
  if (result.IsNothing()) {
    printf("\nCan't instantiate module.\n");
    exit(EXIT_FAILURE);
  }

  // Returning check-out module
  return mod;
}

/*****************************************************************************
 * v8::Local<v8::Value> execModule
 * Executes module's code
 *****************************************************************************/
v8::Local<v8::Value> execModule(v8::Local<v8::Module> mod,
                                v8::Local<v8::Context> cx,
                                bool nsObject) {

  // Executing module with return value
  v8::Local<v8::Value> retValue;
  if (!mod->Evaluate(cx).ToLocal(&retValue)) {
    printf("Error evaluating module!\n");
    exit(EXIT_FAILURE);
  }

  // nsObject determins, if module namespace or return value has to be returned.
  // Module namespace is required during import callback; see lines # and #.
  if (nsObject)
    return mod->GetModuleNamespace();
  else
    return retValue;
}

/*****************************************************************************
 * v8::MaybeLocal<v8::Module> callResolve
 * Callback from static import.
 *****************************************************************************/
v8::MaybeLocal<v8::Module> callResolve(v8::Local<v8::Context> context,
                                       v8::Local<v8::String> specifier,
                                       v8::Local<v8::Module> referrer) {

  // Get module name from specifier (given name in import args)
  v8::String::Utf8Value str(context->GetIsolate(), specifier);

  // Return unchecked module
  return loadModule(readFile(*str), *str, context);
}

/*****************************************************************************
 * v8::MaybeLocal<v8::Promise> callDynamic
 * Callback from dynamic import.
 *****************************************************************************/
v8::MaybeLocal<v8::Promise> callDynamic(v8::Local<v8::Context> context,
                                        v8::Local<v8::ScriptOrModule> referrer,
                                        v8::Local<v8::String> specifier) {

  // Promise resolver: that way promise for dynamic import can be rejected
  // or full-filed
  v8::Local<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(context).ToLocalChecked();
  v8::MaybeLocal<v8::Promise> promise(resolver->GetPromise());

  // Loading module (with checking)
  v8::String::Utf8Value name(context->GetIsolate(), specifier);
  v8::Local<v8::Module> mod =
      checkModule(loadModule(readFile(*name), *name, context), context);
  v8::Local<v8::Value> retValue = execModule(mod, context, true);

  // Resolving (fulfilling) promise with module global namespace
  resolver->Resolve(context, retValue);
  return promise;
}

/*****************************************************************************
 * void callMeta
 * Callback for module metadata. 
 *****************************************************************************/
void callMeta(v8::Local<v8::Context> context,
              v8::Local<v8::Module> module,
              v8::Local<v8::Object> meta) {

  // In this example, this is throw-away function. But it shows that you can
  // bind module's url. Here, placeholder is used.
  meta->Set(
      context,
      v8::String::NewFromUtf8(context->GetIsolate(), "url").ToLocalChecked(),
      v8::String::NewFromUtf8(context->GetIsolate(), "https://something.sh")
          .ToLocalChecked());
}
