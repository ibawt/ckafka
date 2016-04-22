require 'spec_helper'

describe Ckafka do
  it 'has a version number' do
    expect(Ckafka::VERSION).not_to be nil
  end

  it 'should close and open' do
    Ckafka.close()
    Ckafka.init()
    Ckafka.close()
    Ckafka.init()
    Ckafka.close()
  end

  describe 'add_broker' do
    it 'should raise when passed not a string' do
      expect{ Ckafka.add_broker(nil) }.to raise_error(TypeError)
      expect{ Ckafka.add_broker(56) }.to raise_error(TypeError)
    end

    it 'should raise something else when passed a string' do
      expect{ Ckafka.add_broker("foo").to raise_error(StandardError)}
    end
  end
end
