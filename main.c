#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

const int sample_rate = 44100;
struct note {
	double start; // in seconds
	double dur; // in seconds
	double freq; // in Hz, i think?
};

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

double envelope(int sample, int len, double rate) {
	double atk = 0.1;
	double dec = 0.1;
	int to_atk = (int) (rate * atk);
	int to_dec = (int) (rate * dec);
	if(sample < to_atk) {
		return ((double) sample) / ((double) to_atk);
	} else if(sample > (len - to_dec)) {
		return ((double) (len - sample)) / ((double) to_dec);
	}
	return 1;
}

void export(FILE * file, int sample_rate, int number_of_samples, int16_t * samples) {
	int fmt_chunk_size = 16;
	int bits_per_sample = 16;
	int audio_format = 1;
	int channels = 1;
	//int number_of_samples = (int) (((double) sample_rate) * duration);
	int byte_rate = (sample_rate * channels * bits_per_sample) / 8;
	int block_align = (channels * bits_per_sample) / 8;
	int data_size = (number_of_samples * channels * bits_per_sample) / 8;
	
	if(!file) { return; }
	
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
		write_u16_le(file, samples[i]);
	}
}

double total_run_length(int num_notes, struct note * notes) {
	double l = 0;
	for(int i = 0; i < num_notes; i ++) {
		double k = notes[i].start + notes[i].dur; // no ovrscan 4 now
		if(k > l) { l = k; }
	}
	return l;
}

void generate_audio(int16_t * datas, double start, int sample_rate, int len, struct note * notes, int num_notes) {
	int t;
	double samp;
	double s;
	int the_start;
	int the_end;
	for(int i = 0; i < len; i ++) {
		samp = 0;
		t = i + start;
		for(int j = 0; j < num_notes; j ++) {
			the_start = (int) (notes[j].start * sample_rate);
			the_end = the_start + (int) (notes[j].dur * sample_rate);
			if(t < the_start || t > the_end) { continue; }
			double x = (2 * M_PI * notes[j].freq) / sample_rate;
			double vol = 0.4 * 32767;
			s = vol * sin(x * i);
			s *= envelope(t - the_start, (int) (notes[j].dur * sample_rate), sample_rate);
			samp += s;
		}
		datas[i] = (int) samp;
	}
}

int main(int argc, char ** argv) {
	float F3 = 174.62;
	float G3 = 196.00;
	float A4 = 220.00;
	float B4 = 246.94;
	float C4 = 261.63;
	float D4 = 293.66;
	float E4 = 329.63;
	float F4 = 349.23;
	float G4 = 392.00;
	float A5 = 440.00;
	float B5 = 493.88;
	int num_notes = 16;
	struct note notes[16] = {
		{ 0.00, 0.25, G3},
		{ 0.25, 0.25, B4},
		{ 0.50, 0.25, C4},
		{ 0.75, 0.75, D4},

		{ 2.00, 0.25, G3},
		{ 2.25, 0.25, B4},
		{ 2.50, 0.25, C4},
		{ 2.75, 0.75, D4},

		{ 4.00, 0.25, G3},
		{ 4.25, 0.25, B4},
		{ 4.50, 0.25, C4},
		{ 4.75, 0.50, D4},
		{ 5.25, 0.50, B4},
		{ 5.75, 0.50, G3},
		{ 6.25, 0.50, B4},
		{ 6.75, 1.50, A4},
	};
	
	int len = (int) (((double) sample_rate) * total_run_length(num_notes, notes));
	int16_t * samples = malloc(sizeof(int16_t) * len);
	generate_audio(samples, 0, sample_rate, len, notes, num_notes);
	
	FILE * file = fopen("melody.wav", "wb");
	export(file, sample_rate, len, samples);
	fclose(file);
	
	free(samples);
	return 0;
}

