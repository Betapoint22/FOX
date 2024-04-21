#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define CLIENT_PORT 12345
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define BYTES_PER_SAMPLE 3  // 24-bit audio

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void capture_and_send_audio(char *host, int port) {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int dir;
    char *buffer;
    int buffer_size;

    // Open PCM device for recording
    if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0) < 0) {
        error("ERROR: Capture open error");
    }

    // Allocate parameters object and fill it with default values
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    // Set parameters: interleaved mode, sample format, sample rate, number of channels
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE); // 24-bit little-endian
    snd_pcm_hw_params_set_rate_near(handle, params, &SAMPLE_RATE, &dir);
    snd_pcm_hw_params_set_channels(handle, params, NUM_CHANNELS);

    // Write parameters to the driver
    snd_pcm_hw_params(handle, params);

    // Allocate buffer to hold single period
    snd_pcm_hw_params_get_period_size(params, &buffer_size, &dir);
    buffer = (char *)malloc(buffer_size);

    // Create UDP socket
    int sockfd;
    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("ERROR opening socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        error("Invalid address/ Address not supported");
    }

    // Capture and send audio data
    while (1) {
        snd_pcm_readi(handle, buffer, buffer_size);
        sendto(sockfd, buffer, buffer_size, 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }

    // Close PCM handle and free buffer
    snd_pcm_close(handle);
    free(buffer);
}

int main(int argc, char *argv[]) {
    char *host = "127.0.0.1"; // Change to the client's IP address
    int port = CLIENT_PORT;

    // Capture and send audio
    capture_and_send_audio(host, port);

    return 0;
}
