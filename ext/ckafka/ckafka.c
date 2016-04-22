#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ruby.h>

#include "librdkafka/rdkafka.h"

#define MAX_SHUTDOWN_TRIES 5

static rd_kafka_t *rk = NULL;

static void logger(const rd_kafka_t *rk, int level,
                   const char *fac, const char *buf)
{
  fprintf(stderr, "%s: %s\n", fac, buf);
}


static VALUE kafka_send(VALUE self, VALUE topic_value, VALUE key, VALUE message)
{
  rd_kafka_topic_conf_t *topic_conf;
  rd_kafka_topic_t *topic;
  char* topic_name;
  void* message_bytes;
  size_t  message_len;
  int res;
  void *key_buf;
  size_t key_len;

  if (key == Qnil) {
    key_buf = NULL;
    key_len = 0;
  } else {
    key_buf = RSTRING_PTR(key);
    key_len = RSTRING_LEN(key);
  }

  topic_name = StringValueCStr(topic_value);
  if (!topic_name) {
    rb_raise(rb_eStandardError, "topic is not a string!");
  }

  message_bytes = RSTRING_PTR(message);
  if(!message_bytes) {
    rb_raise(rb_eStandardError, "failed to get message ptr");
  }

  message_len = RSTRING_LEN(message);

  topic_conf = rd_kafka_topic_conf_new();
  if(!topic_conf) {
    rb_raise(rb_eStandardError, "failed to create kafka topic configuration");
  }

  topic = rd_kafka_topic_new(rk, topic_name, topic_conf);
  if(!topic) {
    rb_raise(rb_eStandardError, "failed to create topic");
  }

  res = rd_kafka_produce(topic, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY, message_bytes, message_len,
                         key_buf, key_len, NULL);

  rd_kafka_topic_destroy(topic);

  if (res) {
    rb_raise(rb_eStandardError, "rd_kafka_produce failed: %d", res);
  }

  return Qnil;
}

static VALUE kafka_destroy()
{
  if(rk) {
    int i;

    for ( i = 0 ; i < MAX_SHUTDOWN_TRIES ; ++i ) {
      if (rd_kafka_outq_len(rk) <= 0 ) {
        break;
      } else {
        rd_kafka_poll(rk, 100);
      }
    }

    rd_kafka_destroy(rk);
    rk = NULL;
  }
  return Qnil;
}

static VALUE kafka_add_broker(VALUE self, VALUE broker)
{
  char *value = StringValueCStr(broker);
  int res;

  if(!value) {
    rb_raise(rb_eArgError, "invalid string");
  }

  res = rd_kafka_brokers_add(rk, value);
  if (res == 0) {
    rb_raise(rb_eStandardError, "failed to add any brokers!");
  }

  return Qnil;
}

static VALUE kafka_init()
{
  rd_kafka_conf_t *conf;
  char errstr[512];

  if(!rk) {
    kafka_destroy();
  }
  conf = rd_kafka_conf_new();

  rd_kafka_conf_set_log_cb(conf, logger);

  rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!rk) {
    rb_raise(rb_eStandardError, "failed to create kafka producer: %s\n", errstr);
  }
  return Qnil;
}

VALUE Init_ckafka()
{
  VALUE kafka_module = rb_define_module("Ckafka");

  rb_define_singleton_method(kafka_module, "init", kafka_init, 0);
  rb_define_singleton_method(kafka_module, "produce", kafka_send, 3);
  rb_define_singleton_method(kafka_module, "add_broker", kafka_add_broker, 1);
  rb_define_singleton_method(kafka_module, "close", kafka_destroy, 0);

  kafka_init();

  return Qnil;
}
