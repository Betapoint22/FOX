#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define BYTES_PER_SAMPLE 3  // 24-bit audio
#define DURATION 5  // Duration of audio capture in seconds

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void receive_audio_over_tcp(char *host, int port, char *output_path) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("ERROR opening socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        error("Invalid address/ Address not supported");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

    // Receive audio data
    FILE *fp = fopen(output_path, "wb");
    int bytes_received = 0;

    while (bytes_received < SAMPLE_RATE * DURATION * BYTES_PER_SAMPLE) {
        int bytes = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes < 0) {
            error("ERROR receiving data");
        }
        bytes_received += bytes;
        fwrite(buffer, 1, bytes, fp);
    }

    fclose(fp);
    close(sockfd);
}

void enhance_audio(char *input_path, char *output_path) {
    // Here you can implement your audio enhancement algorithm
    // For simplicity, let's just copy the input to the output
    FILE *fin = fopen(input_path, "rb");
    FILE *fout = fopen(output_path, "wb");

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fin)) > 0) {
        fwrite(buffer, 1, bytes_read, fout);
    }

    fclose(fin);
    fclose(fout);
}

void play_audio(char *audio_path) {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
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
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    buffer_size = frames * NUM_CHANNELS * BYTES_PER_SAMPLE; // Size in bytes
    buffer = (char *)malloc(buffer_size);

    // Open audio file for playback
    FILE *fp = fopen(audio_path, "rb");
    if (fp == NULL) {
        error("ERROR: Could not open audio file for playback");
    }

    // Play audio
    while (fread(buffer, 1, buffer_size, fp) > 0) {
        snd_pcm_writei(handle, buffer, frames);
    }

    // Close PCM handle and free buffer
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    fclose(fp);
    free(buffer);
}

int main(int argc, char *argv[]) {
    char *host = "127.0.0.1"; // Change to the server's IP address
    char *input_file = "received_audio.raw";
    char *output_file = "enhanced_audio.raw";

    while (1) {
        // Receive audio over TCP
        receive_audio_over_tcp(host, SERVER_PORT, input_file);

        // Enhance the received audio
        enhance_audio(input_file, output_file);

        // Play enhanced audio
        play_audio(output_file);
    }

    return 0;
}
