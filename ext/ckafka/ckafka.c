#include <ruby.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "librdkafka/rdkafka.h"

static rd_kafka_t *rk = NULL;
static VALUE kafka_module;
static void logger(const rd_kafka_t *rk, int level,
                   const char *fac, const char *buf)
{
  fprintf(stderr, "%s: %s\n", fac, buf);
}


VALUE ckafka_send(VALUE topic_value, VALUE message, VALUE metadata)
{
  rd_kafka_topic_conf_t *topic_conf;
  rd_kafka_topic_t *topic;
  char* topic_name;
  void* message_bytes = NULL;
  size_t  message_len;
  int res;

  topic_name = StringValueCStr(topic_value);
  message_bytes = RSTRING_PTR(message_bytes);
  message_len = RSTRING_LEN(message);

  topic_conf = rd_kafka_topic_conf_new();

  topic = rd_kafka_topic_new(rk, topic_name, topic_conf);

  res = rd_kafka_produce(topic, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY, message_bytes, message_len,
                         NULL, 0, NULL);

  rd_kafka_topic_destroy(topic);
  return Qnil;
}

void ckafka_destroy(void)
{
  if(rk) {
    while( rd_kafka_outq_len(rk) > 0 ) {
      rd_kafka_poll(rk, 100);
    }

    rd_kafka_destroy(rk);
    rk = NULL;
  }
}

static VALUE add_broker(VALUE broker)
{
  char *value = StringValueCStr(broker);
  if(!value) {
    return Qnil;
  }

  int res = rd_kafka_brokers_add(rk, value);
  if (res == 0) {
    fprintf(stderr, "invalid brokers!\n");
  }

  return Qnil;
}

void Init_ckafka(void)
{
  rd_kafka_conf_t *conf;
  char errstr[512];

  conf = rd_kafka_conf_new();

  rd_kafka_conf_set_log_cb(conf, logger);

  rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!rk) {
    fprintf(stderr, "failed to create kafka producer: %s\n", errstr);
    return;
  }

  kafka_module = rb_define_module("Ckafka");

  rb_define_singleton_method(kafka_module, "kafka_send", ckafka_send, 3);
  rb_define_singleton_method(kafka_module, "add_broker", add_broker, 1)
  atexit(ckafka_destroy);
}
