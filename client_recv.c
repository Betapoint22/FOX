#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define BYTES_PER_SAMPLE 3  // 24-bit audio

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void receive_and_play_audio(int port) {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int dir;
    char *buffer;
    int buffer_size;

    // Open PCM device for playback
    if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        error("ERROR: Playback open error");
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
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("ERROR opening socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind socket
    if (bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // Receive and play audio data
    while (1) {
        int bytes = recvfrom(sockfd, buffer, buffer_size, 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (bytes < 0) {
            error("ERROR receiving data");
        }
        snd_pcm_writei(handle, buffer, bytes / BYTES_PER_SAMPLE / NUM_CHANNELS);
    }

    // Close PCM handle and free buffer
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
}

int main(int argc, char *argv[]) {
    // Receive and play audio
    receive_and_play_audio(SERVER_PORT);

    return 0;
}
