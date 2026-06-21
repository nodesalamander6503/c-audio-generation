#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

double lookup_base_freq_4(int note, int accidental) {
	// this whole function is just copy-pasted from some tiny AI model
	// because i never learned how music works
	// i will replace it with my own code once i learn music lol
	// for now, i think this code is stylistically different but otherwise okay?
	if(note == 'R') { return 0.0; }
    switch (note) {
        case 'C':
            if (accidental == 0)   return 261.63;
            if (accidental == '#') return 277.18; // C# = Db
            if (accidental == 'b') return 246.94; // Cb = B3, awkward but valid-ish
            break;

        case 'D':
            if (accidental == 0)   return 293.66;
            if (accidental == '#') return 311.13; // D# = Eb
            if (accidental == 'b') return 277.18; // Db = C#
            break;

        case 'E':
            if (accidental == 0)   return 329.63;
            if (accidental == '#') return 349.23; // E# = F
            if (accidental == 'b') return 311.13; // Eb = D#
            break;

        case 'F':
            if (accidental == 0)   return 349.23;
            if (accidental == '#') return 369.99; // F# = Gb
            if (accidental == 'b') return 329.63; // Fb = E
            break;

        case 'G':
            if (accidental == 0)   return 392.00;
            if (accidental == '#') return 415.30; // G# = Ab
            if (accidental == 'b') return 369.99; // Gb = F#
            break;

        case 'A':
            if (accidental == 0)   return 440.00;
            if (accidental == '#') return 466.16; // A# = Bb
            if (accidental == 'b') return 415.30; // Ab = G#
            break;

        case 'B':
            if (accidental == 0)   return 493.88;
            if (accidental == '#') return 523.25; // B# = C5
            if (accidental == 'b') return 466.16; // Bb = A#
            break;
    }

    fprintf(stderr, "invalid pitch\n");
    exit(1);
}
void add_note(struct note ** notes, int * num_notes, struct note * pattern) {
	(*num_notes)++;
	*notes = realloc(*notes, sizeof(struct note) * (*num_notes));
	memcpy(&((*notes)[(*num_notes) - 1]), pattern, sizeof(struct note));
}
void parse_notes(int * num_notes, struct note ** notes, char * code) {
	double t = 0;
	double bps = 4;    // beats per sec
	double bl = 1/bps; // beat len
	int octave = 4;
	for(int i = 0; code[i] != 0; i++) {
		if(code[i] == ' ') { continue; }

		int note = code[i++];

		int accidental = 0;
		if(code[i] == 'b' || code[i] == '#') {
			accidental = code[i++];
			if (code[i] == 0) { goto error; }
		}
	
		if(note != 'R') {
				if(code[i] < '0' || code[i] > '9') { goto error; }
				octave = code[i++] - '0';
		}
	
		double duration = 1.0;
		if(code[i] == ':') {
			i++;
			if (code[i] < '0' || code[i] > '9') { goto error; }
			duration = code[i++] - '0';
		}

		if(code[i] != 0 && code[i] != ' ') { goto error; }

		double freq = lookup_base_freq_4(note, accidental) * pow(2.0, octave - 4);
		struct note note_defn;
		note_defn.start = t;
		note_defn.dur = duration * bl;
		note_defn.freq = freq;
		add_note(notes, num_notes, &note_defn);

		t += bl * duration;

		if(code[i] == 0) { break; }
	}

	return;

error:
	fprintf(stderr, "parser error -- invalid note format\n");
	exit(1);
}

int main(int argc, char ** argv) {
	int num_notes = 0;
	struct note * notes = NULL;
	parse_notes(&num_notes, &notes, argv[1]);
	//"Gb3 Bb4 C4 D4:3 Gb3 Bb4 C4 D4:3 Gb3 Bb4 C4 D4:2 Bb4:2 Gb3:2 Bb4:2 A4:6");
	
	int len = (int) (((double) sample_rate) * total_run_length(num_notes, notes));
	int16_t * samples = malloc(sizeof(int16_t) * len);
	generate_audio(samples, 0, sample_rate, len, notes, num_notes);
	
	FILE * file = fopen("melody.wav", "wb");
	export(file, sample_rate, len, samples);
	fclose(file);
	
	free(samples);
	return 0;
}

