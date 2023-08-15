/*
 * Copyright 2017 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBASE_APP_REST_RESPONSE_STREAM_H_
#define FIREBASE_APP_REST_RESPONSE_STREAM_H_

#include <string>
#include <utility>

#include "app/rest/response.h"
#include "app/src/assert.h"
#include "app/src/log.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/stl_emulation.h"

namespace firebase {
namespace rest {

// TODO(almostmatt): better comment for response stream
// HTTP/REST response with Content-Type: application/json. FbsType is FlatBuffer
// type that contains the application data. Before the conversion between JSON
// and FlexBuffers are supported, we need to specify FlatBuffers type here.
//
// See response_stream_test.cc for an example using this class.
class ResponseStream : public Response {
 public:

  // Note(almostmatt): using curl_requests notifer as a rough example for callback typedef
  // also note that this is expected to be an internal concept.
  // Used to notify a subscriber of an update to the response state,
  // in notifer, UpdateCallbackType is an enum of complete/fail/progress etc
  // curiously the notifier callback takes void* data and does not react to the progress or the response at all
  // unless the data contains a pointer to the response I guess.
  // TODO(almostmatt): decide if I want to call this with char*, lenght or string or other
  typedef void (*UpdateCallback)(std::string body_chunk, void* callback_data);

  // TODO(almostmatt): maybe add constructor with callback instead of set_callback method
  // TODO(almostmatt): maybe add copy/move constructors. some comment in other class about vs 2015 and also some macro in blockingresponse or other class

  // Set the callback to be notified about this object.
  // Note(almostmatt): could also add a void* callback_data in order to 
  // have some object always available, like a pointer to remote_config
  void set_update_callback(UpdateCallback callback, void* callback_data) {
    update_callback_ = callback;
    update_callback_data_ = callback_data;
  }

  // Note(almostmatt): default MarkCompleted and MarkFailed are fine for now.
  // When the response fails, ensure that application_data_ is set.
  bool ProcessBody(const char* buffer, size_t length) override {
    // TODO(almostmatt): notify progress (buffer, length)
    bool result = Response::ProcessBody(buffer, length);
    if (result && update_callback_) {
      // Since buffer may NOT neccessarily end with \0, pass in length in the init.
      std::string body_chunk(buffer, length);
      update_callback_(body_chunk, update_callback_data_);
    }
    return result;
  }
private:
  UpdateCallback update_callback_;
  void* update_callback_data_;
};

}  // namespace rest
}  // namespace firebase

#endif  // FIREBASE_APP_REST_RESPONSE_STREAM_H_
