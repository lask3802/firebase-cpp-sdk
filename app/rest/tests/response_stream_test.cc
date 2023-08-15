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

#include "app/rest/response_stream.h"

#include <chrono>
#include <cstring>
#include <thread>

#include "app/src/log.h"
#include "app/rest/transport_curl.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace firebase {
namespace rest {
// A helper function that prepares char buffer and calls the response's
// ProcessHeader. Param str must be a C string.
void ProcessHeader(const char* str, Response* response) {
  // Prepare the char buffer to call ProcessHeader and make sure the
  // implementation does not rely on that buffer is \0 terminated.
  size_t length = strlen(str);
  char* buffer = new char[length + 20];  // We pad the buffer with a few '#'.
  memset(buffer, '#', length + 20);
  memcpy(buffer, str, length);  // Intentionally not copy the \0.

  // Now call the ProcessHeader.
  response->ProcessHeader(buffer, length);
  delete[] buffer;

}

class ResponseStreamTest : public testing::Test {
 protected:
  void SetUp() override {
    LogWarning("almostmatt - setup");
    InitTransportCurl();
  }
  void TearDown() override {
    LogWarning("almostmatt - teardown");
    CleanupTransportCurl();
  }
  // static int32_t port_;
  // static net_http2::HTTPServer2* server_;
};

/*
TEST_F(ResponseStreamTest, ProcessBody) {
  ResponseStream response;
  std::vector<std::string> body_chunks;
  response.set_update_callback(
    [](std::string body_chunk, void* update_data){
      auto body_chunks = (std::vector<std::string>*)update_data;
      body_chunks->push_back(body_chunk);},
    (void*)&body_chunks);

  EXPECT_EQ(0, response.status());

  ProcessHeader("HTTP/1.1 200 OK\r\n", &response);
  EXPECT_EQ(200, response.status());

  const char body[] =
      "{"
      "}";
  response.ProcessBody(body, sizeof(body));
  response.ProcessBody(body, sizeof(body));
  response.MarkCompleted();
  EXPECT_TRUE(response.header_completed());
  EXPECT_TRUE(response.body_completed());

  ASSERT_EQ(2, body_chunks.size());
  EXPECT_STREQ(body, body_chunks[0]);
  EXPECT_STREQ(body, body_chunks[1]);
  // NOTE(almostmatt): chunks might not be null terminated, but strings are.
  LogWarning("almostmatt - end test");
}
*/

TEST_F(ResponseStreamTest, ServerRequest) {
  // Note: almostmatt - theoretically I can use this response type for a non-streamed response
  // and it would still report each chunk as it comes.
  // but I want to test a streamed response and verify at least N chunks.

  LogWarning("almostmatt - start test");
  int chunks_to_fetch = 10;
  
  Request request;
  ResponseStream response;
  request.set_verbose(true);

  std::vector<std::string> body_chunks;
  LogWarning("almostmatt - setting callback");
  response.set_update_callback(
    [](std::string body_chunk, void* update_data){
      LogWarning("almostmatt - found a response chunk:");
      LogWarning(body_chunk.c_str());
      auto body_chunks = (std::vector<std::string>*)update_data;
      body_chunks->push_back(body_chunk);
      if (body_chunks->size() >= 10) {
        // TODO: is there any good way to kill the stream from this callback?
      }
    },
    (void*)&body_chunks);

  // EXPECT_EQ(0, response.status());
  // ProcessHeader("HTTP/1.1 200 OK\r\n", &response);

  // to generalize this test, would like to talk to a local test server
  // TransportCurlTest looks extremely promising as an example of this.
  // it talks to localhost on some previously unused port.

  // oh, sad day, firebase_app_rest_transport_curl_test is commented out due to
  // google3 dependencies :(

  LogWarning("almostmatt - building url");
  //const std::string& url = "http://www.google.com";
  const std::string& url = "http://anglesharp.azurewebsites.net/Chunked";
  // called to google and it gets the response over many chunks.
  // const std::string& url = "http://localhost:" + std::to_string(4379);
  //    absl::StrFormat("http://localhost:%d", 4379);
  // absl::StrFormat("http://localhost:%d", TransportCurlTest::port_);
  request.set_url(url.c_str());

  EXPECT_EQ(0, response.status());
  EXPECT_FALSE(response.header_completed());
  EXPECT_FALSE(response.body_completed());
  EXPECT_EQ(nullptr, response.GetHeader("Server"));
  EXPECT_STREQ("", response.GetBody());

  LogWarning("almostmatt - perform curl");
  TransportCurl curl;
  curl.set_is_async(true);
  curl.Perform(request, &response);

  LogWarning("almostmatt - sleep start");
  // almostmatt - generalize this better later
  // Note: does curl.Perform block until the response is closed?
  std::this_thread::sleep_for(std::chrono::milliseconds(600));
  // one of the response chunks is delayed by 1s
  // but header is probably complete by then.
  // Expect at least the header and a body chunk
  EXPECT_EQ(200, response.status());
  EXPECT_TRUE(response.header_completed());
  ASSERT_LT(0, body_chunks.size());
  EXPECT_FALSE(response.body_completed());

  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  LogWarning("almostmatt - sleep end");
  EXPECT_EQ(200, response.status());

  EXPECT_TRUE(response.header_completed());
  EXPECT_TRUE(response.body_completed());


  // EXPECT_STREQ(kServerVersion, response.GetHeader("Server"));
  // EXPECT_STREQ("test", response.GetBody());

  ASSERT_LT(1, body_chunks.size());
  EXPECT_EQ("<h5>This is a chunked response after 100 ms.</h5>", body_chunks[1]);

  LogWarning("almostmatt - response full header is:");
  std::map<std::string, std::string> header_entries = response.GetHeaderMap();
  for (auto entry : header_entries) {
    LogWarning("  %s: %s", entry.first.c_str(), entry.second.c_str());
  }
  LogWarning("almostmatt - response full body is:");
  LogWarning(response.GetBody());

  // NOTE(almostmatt): chunks might not be null terminated, but strings are.
  
  // TODO(almostmatt): look into request/response timeout ms
  // either set it large or set it infinite (or make it only timeout if no progress is made)
  // check realtime rc specs.

  // TODO: with async request, try cancelling the request
}

// Note: almostmatt - theoretically either of client or server can close a stream
// and I should be able to see if it succeeded (closed by server), or failed (network error), or cancelled (closed by client)

// TODO(almostmatt): Have a unit test use real transport_curl and talk to
// a real server with a dummy stream.
// (Host the stream locally if needed)


/*
almostmatt: the following Wait is copied from transport_curl's TestResponse class.

  void Wait() {
    absl::MutexLock lock(&mutex_);
    mutex_.AwaitWithTimeout(
        absl::Condition(
            [](void* userdata) -> bool {
              auto* response = static_cast<TestResponse*>(userdata);
              return response->header_completed() && response->body_completed();
            },
            this),
        kTimeoutSeconds);
  }

 private:
 // matt: mutex can be used so that MarkCompleted and body_completed are not called at once.
  absl::Mutex mutex_;
*/

}  // namespace rest
}  // namespace firebase
