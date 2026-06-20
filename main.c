#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

void write_u16_le(FILE *f, uint16_t x) {
    fputc(x & 0xff, f);
    fputc((x >> 8) & 0xff, f);
}

void write_u32_le(FILE *f, uint32_t x) {
    fputc(x & 0xff, f);
    fputc((x >> 8) & 0xff, f);
    fputc((x >> 16) & 0xff, f);
    fputc((x >> 24) & 0xff, f);
}

float envelope(int sample, int len, float rate) {
	float atk = 0.1;
	float dec = 0.1;
	int to_atk = (int) (rate * atk);
	int to_dec = (int) (rate * dec);
	if(sample < to_atk) {
		return ((float) sample) / ((float) to_atk);
	} else if(sample > (len - to_dec)) {
		return ((float) (len - sample)) / ((float) to_dec);
	}
	return 1;
}

int main(int argc, char ** argv) {
	int fmt_chunk_size = 16;
	int bits_per_sample = 16;
	int audio_format = 1;
	int channels = 1;
	int sample_rate = 44100;
	int number_of_samples = sample_rate * 9;
	int byte_rate = (sample_rate * channels * bits_per_sample) / 8;
	int block_align = (channels * bits_per_sample) / 8;
	int data_size = (number_of_samples * channels * bits_per_sample) / 8;
	
	int16_t * datas = malloc(sizeof(int16_t) * number_of_samples);
	float amplitude = 0.3 * 32767.0;
	 
	float A4 = 440.00;
	float B4 = 493.88;
	float C4 = 261.63;
	float D4 = 293.66;
	float E4 = 329.63;
	float F4 = 349.23;
	float G4 = 392.00;

	float freqs[64] = {
		C4, E4, F4, G4, 0, 0,
		C4, E4, F4, G4, 0, 0,
		E4, E4, D4, G4, 0, 0,
		E4, G4, G4, F4, 0, 0,
		E4, F4, G4,  0, 0, 0,
		E4, C4, D4, C4, 0, 0,
	};
	for(int i = 0; i < number_of_samples; i ++) {
		int index = (int) (i*4/sample_rate);
		float freq = freqs[index];
		float x = (2 * M_PI * freq) / sample_rate;
		double mask = envelope(i - (int) ((index * sample_rate)/4), sample_rate / 4, sample_rate);
		double sample = mask * amplitude * sin(x * i);
		datas[i] = (int) sample;
	}
	
	FILE * file = fopen("uwu.wav", "wb");
	if(!file) { return 1; }
	fwrite("RIFF", 1, 4, file);
	write_u32_le(file, 36 + data_size);
	fwrite("WAVE", 1, 4, file);
	fwrite("fmt ", 1, 4, file);
	write_u32_le(file, fmt_chunk_size);
	write_u16_le(file, audio_format);
	write_u16_le(file, channels);
	write_u32_le(file, sample_rate);
	write_u32_le(file, byte_rate);
	write_u16_le(file, block_align);
	write_u16_le(file, bits_per_sample);
	fwrite("data", 1, 4, file);
	write_u32_le(file, data_size);
	for(int i = 0; i < number_of_samples; i ++) {
		write_u16_le(file, datas[i]);
	}
	fclose(file);
	
	free(datas);
	return 0;
}

