// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include "rsocket/internal/Common.h"
#include "rsocket/statemachine/StreamFragmentAccumulator.h"
#include "yarpl/Flowable.h"
#include "yarpl/Single.h"

namespace folly {
class IOBuf;
}

namespace rsocket {

class StreamsWriter;
struct Payload;

/// A common base class of all state machines.
///
/// The instances might be destroyed on a different thread than they were
/// created.
class StreamStateMachineBase {
 public:
  StreamStateMachineBase(
      std::shared_ptr<StreamsWriter> writer,
      StreamId streamId)
      : writer_(std::move(writer)), streamId_(streamId) {}
  virtual ~StreamStateMachineBase() = default;

  virtual void handlePayload(
      Payload&& payload,
      bool complete,
      bool flagsNext,
      bool flagsFollows) = 0;
  virtual void handleRequestN(uint32_t n);
  virtual void handleError(Payload errorPayload);
  virtual void handleCancel();

  virtual size_t getConsumerAllowance() const;

  /// Indicates a terminal signal from the connection.
  ///
  /// This signal corresponds to Subscriber::{onComplete,onError} and
  /// Subscription::cancel.
  /// Per ReactiveStreams specification:
  /// 1. no other signal can be delivered during or after this one,
  /// 2. "unsubscribe handshake" guarantees that the signal will be delivered
  ///   exactly once, even if the state machine initiated stream closure,
  /// 3. per "unsubscribe handshake", the state machine must deliver
  /// corresponding
  ///   terminal signal to the connection.
  virtual void endStream(StreamCompletionSignal) {}

 protected:
  void
  newStream(StreamType streamType, uint32_t initialRequestN, Payload payload);

  void writeRequestN(uint32_t);
  void writeCancel();

  void writePayload(Payload&& payload, bool complete = false);
  void writeComplete();
  void writeApplicationError(folly::StringPiece);
  void writeInvalidError(folly::StringPiece);

  void removeFromWriter();

  std::shared_ptr<yarpl::flowable::Subscriber<Payload>> onNewStreamReady(
      StreamType streamType,
      Payload payload,
      std::shared_ptr<yarpl::flowable::Subscriber<Payload>> response);

  void onNewStreamReady(
      StreamType streamType,
      Payload payload,
      std::shared_ptr<yarpl::single::SingleObserver<Payload>> response);

  /// A partially-owning pointer to the connection, the stream runs on.
  /// It is declared as const to allow only ctor to initialize it for thread
  /// safety of the dtor.
  const std::shared_ptr<StreamsWriter> writer_;
  StreamFragmentAccumulator payloadFragments_;

 private:
  const StreamId streamId_;
};

} // namespace rsocket
