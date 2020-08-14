from confluent_kafka import Producer


def acked(err, msg):
    if err is not None:
        print("Failed to deliver message: %s: %s" % (str(msg), str(err)))
    else:
        print("Message produced: %s" % (str(msg)))

if __name__ == "__main":
    conf = {'bootstrap.servers': "10.230.249.72:9092",
            'client.id': "s3bg-test"}
    producer = Producer(conf)

    producer.produce("test", key="s3background-test", value="value", callback=acked)
    # Wait up to 1 second for events. Callbacks will be invoked during
    # this method call if the message is acknowledged.
    producer.poll(1)
