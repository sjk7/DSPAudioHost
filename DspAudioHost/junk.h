#pragma once
/*/
      portaudio_cpp::test testy;
  testy.add();
  portaudio_cpp::test testy2 = testy;

      bool querySupportedSamplerate(
      PaStreamParameters* in, const int samplerate, PaStreamParameters* out) {
      const auto err = Pa_IsFormatSupported(in, out, samplerate);
      if (err == paFormatIsSupported) return true;
      return false;
  }
  void queryAvailableAudioProps() {
      supportedInputInfo.clear();
      supportedInputInfo.insert({1, 1});

      PaStreamParameters ip = prepareInputParams(
          this->index, SampleFormat::Float32, info.defaultLowInputLatency);
      PaStreamParameters op = prepareOutputParams(
          this->index, SampleFormat::Float32, info.defaultLowInputLatency);

      if (info.maxInputChannels > 0) {

          for (auto&& sr : allowed_samplerates) {
              bool b = querySupportedSamplerate(&ip, sr, nullptr);
              if (b) {
                  SupportedInfoData d{sr, SampleFormat::Float32};
                  supportedInputInfo.data.insert({DuplexTypes::input, d});
              }
          }

          for (auto&& sr : allowed_samplerates) {
              bool b = querySupportedSamplerate(&ip, sr, &op);
              if (b) {
                  SupportedInfoData d{sr, SampleFormat::Float32};
                  supportedInputInfo.data.insert({DuplexTypes::duplex, d});
              }
          }
      }

      if (info.maxOutputChannels > 0) {
          for (auto&& sr : allowed_samplerates) {
              bool b = querySupportedSamplerate(nullptr, sr, &op);
              if (b) {
                  SupportedInfoData d{sr, SampleFormat::Float32};
                  supportedOutputInfo.data.insert({DuplexTypes::output, d});
              }
          }

          for (auto&& sr : allowed_samplerates) {
              bool b = querySupportedSamplerate(&ip, sr, &op);
              if (b) {
                  SupportedInfoData d{sr, SampleFormat::Float32};
                  supportedOutputInfo.data.insert({DuplexTypes::duplex, d});
              }
          }
      }

}

const SupportedInfo&
       queryDeviceProperties() {
  queryAvailableAudioProps();
  return supportedInputInfo;
}
      /*/
