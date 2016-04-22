require 'mkmf'

dir_config('ckafka')

unless find_library('rdkafka', 'rd_kafka_topic_new')
  abort "Cannot find librdkafka"
end

create_makefile('ckafka/ckafka')
