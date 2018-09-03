﻿#include"mfcc.h"

/////////////////////measuring scale/////////////////////////////////
float mel2hz(float mel)
{
	return (700 * (exp(mel / 1125) - 1));
}

float hz2mel(float hz)
{
	return (2595 * log10(1 + hz / 700));
}

/////////////////////////////////Matrix Processing////////////////////////////
hyper_vector multiply(hyper_vector matrix1, hyper_vector matrix2)
{
	float* matrix;
	int r1 = matrix1.row, c1 = matrix1.col;
	int r2 = matrix2.row, c2 = matrix2.col;
	int i, j, k;
	matrix = (float*)malloc(sizeof(float)*r1*c2);

	for (i = 0; i < r1; i++)
		for (j = 0; j < c2; j++)
			matrix[i*c2 + j] = 0;

	for (i = 0; i < r1; i++)
	{
		for (j = 0; j < c2; j++)
		{
			float sum = 0;
			for (k = 0; k < c1; k++)
				sum = sum + matrix1.data[i*c1 + k] * matrix2.data[k*c2 + j];
			matrix[i*c2 + j] = 20 * log10(sum);
			//if (i*c2 + j > 3190) {
			//printf("%d %.2f ", i*c2 + j, matrix[i*c2 + j]);
			//}
		}
		//printf("\n");
	}
	return setHVector(matrix, c2, r1, 1);
}

hyper_vector transpose(hyper_vector matrix)
{
	int r = matrix.col;
	int c = matrix.row;
	float* transposeMatrix = (float*)malloc(sizeof(float)*c*r);
	int i, j;

	for (int i = 0; i < r; i++)
	{
		for (int j = 0; j < c; j++)
		{
			transposeMatrix[i*c + j] = matrix.data[j*r + i];
			//printf("%f ", transposeMatrix[i*c + j]);
		}
		//printf("\n");
	}

	return setHVector(transposeMatrix, c, r, 1);
}

////////////////////////////////////////////////////////////////

SIGNAL silence_trim(SIGNAL a)
{
	int end, start, dem = 0;
	int size;
	SAMPLE *temp = reverse(a);

	start = detect_silence(a.raw_signal, a.signal_length);
	end = detect_silence(temp, a.signal_length);
	start -= 1600;
	end -= 3200;
	size = a.signal_length - start - end;

	float *sample = (float*)malloc(sizeof(float)*size);

	FILE *f = fopen("file3.txt", "w");
	if (f == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}



	for (int i = start; i < a.signal_length - end; i++) {
		sample[dem] = a.raw_signal[i];
		fprintf(f, "%.8f\n", sample[dem]);
		dem++;

	}
	fclose(f);
	free(temp);
	return setSignal(sample, size);
}

int detect_silence(SAMPLE* a, int signal_len)
{
	int trim_ms = 0; // ms
	int chunk_size = 10 * 16;
	while (dBFS(a, trim_ms, chunk_size, -40) && trim_ms < signal_len)
	{
		trim_ms += chunk_size;
	}
	return trim_ms;
}

int dBFS(SAMPLE* raw_signal, int trim_ms, int chunk_size, int silence_threshold) {
	float sum = 0;
	for (int i = trim_ms; i < trim_ms + chunk_size; i++) {
		sum += raw_signal[i] * raw_signal[i];
	}
	sum = sqrt(sum / (chunk_size));
	sum = 20 * log10(sum);
	if (sum < silence_threshold)
		return 1;
	return 0;
}


SAMPLE *reverse(SIGNAL a) {
	SAMPLE t;
	SAMPLE *sample = (SAMPLE*)malloc(sizeof(SAMPLE)*a.signal_length);
	int dem = 0;
	for (int c = a.signal_length - 1; c >= 0; c--) {
		sample[dem] = a.raw_signal[c];
		dem++;
	}
	return sample;
}

hyper_vector cov(hyper_vector mfcc)
{
	int col = mfcc.col;
	int row = mfcc.row;
	int i, j, k;
	float sum;
	float *mean = (float*)malloc(sizeof(float)*col);
	for (i = 0; i < col; i++) {
		sum = 0;
		for (j = 0; j < row; j++) {
			sum += mfcc.data[j*col + i];
		}
		mean[i] = sum / row;
		//printf("%f ", mean[i]);
	}
	//printf("\n");

	float *cov = (float*)malloc(sizeof(float)*col*col);
	sum = 0;
	for (j = 0; j < col; j++) {
		for (k = 0; k < col; k++) {
			for (i = 0; i < row; i++) {
				sum += (mfcc.data[i*col + j] - mean[j])*(mfcc.data[i*col + k] - mean[k]);
			}
			cov[j*col + k] = sum / (row - 1);
			//printf("%f ", cov[j*col + k]);
			sum = 0;
		}
		//printf("\n");
	}


	float size = col * (col + 1) / 2;
	float *final_mfcc = (float*)malloc(sizeof(float)*size);


	int mark = col + 1;
	int col_temp = col;
	int dem = 0, dem2 = 1;
	i = j = dem2 = dem;
	while (i < size)
	{
		if (i == 0) {
			final_mfcc[i] = cov[i];
			dem2++;
		}
		else {
			if (dem2 < col_temp) {
				j += mark;
				final_mfcc[i] = cov[j];
				dem2++;
			}
			else {
				col_temp--;
				dem++;
				j = dem;
				final_mfcc[i] = cov[j];
				dem2 = 1;

				i++;
				continue;
			}
		}

		i++;
	}
	free(cov);
	return setHVector(final_mfcc, size, 1, 1);
}

void normalize(float * data, int row, int col)
{
	FILE *fmean, *fnorl;
	float sum = 0;
	int i, j;
	float *mean = (float*)malloc(sizeof(float)*col);
	float *max_row = (float*)malloc(sizeof(float)*row);

	fmean = fopen("mean.txt", "w");
	for (i = 0; i < col; i++) {
		sum = 0;
		for (j = 0; j < row; j++) {
			sum += data[j*col + i];
		}
		mean[i] = sum / row;
		printf("%f ", mean[i]);
		fprintf(fmean,"%f ", mean[i]);
	}
	fclose(fmean);

	
	float maximum = 0;

	for (i = 0; i < row; i++)
	{
		for (j = 0; j < col; j++)
			if (fabs(data[i*col + j]) > maximum)
				maximum = fabs(data[i*col + j]);

		max_row[i] = maximum;
		printf("%f ", max_row[i]);
		maximum = 0;
	}

	printf("\n");

	fnorl = fopen("normalized.txt", "w");

	for (i = 0; i < row; i++)
	{
		if(i<70)
			fprintf(fnorl, "%d ", 1);
		else if (i<140)
			fprintf(fnorl, "%d ", 2);
		else if (i<210)
			fprintf(fnorl, "%d ", 3);
		else
			fprintf(fnorl, "%d ", 4);
		for (j = 0; j < col; j++) {
			data[i*col + j] = (data[i*col + j] - mean[j]) / max_row[i];
			printf("%f ", data[i*col + j]);
			fprintf(fnorl, "%d:%f ", j+1, data[i*col + j]);
		}
		printf("\n");
		fprintf(fnorl,"\n");
	}

	fclose(fnorl);
	getch();
}

//////////////////////////////////////////////////////
hyper_vector DCT(hyper_vector a, int num_ceps) {
	int i, j, k;
	int len = a.col;
	float *dct = (float*)malloc(sizeof(float)*a.row*num_ceps);
	float *temp = (float*)malloc(sizeof(float)*len);
	float factor = PI / a.col;
	float sum;

	for (k = 0; k < a.row; k++) {
		for (i = 0; i < len; i++) {
			sum = 0;
			for (j = 0; j < len; j++)
				sum += a.data[k*len + j] * cos((j + 0.5) * i * factor);
			temp[i] = sum;
		}
		for (j = 0; j < num_ceps; j++) {
			//if (k == 498) {
			//	printf("cc ");
			//}
			dct[k*num_ceps + j] = temp[j];
			//printf("%.4f  ", dct[k*num_ceps + j]);
		}
		//printf("\n");
	}
	free(temp);
	return setHVector(dct, num_ceps, a.row, 1);
}


/////////////////////////////////MFCCs////////////////////////////////////////
hyper_vector DFT_PowerSpectrum(hyper_vector frame, int pointFFT)
{
	float temp, real = 0, img = 0;

	hyper_vector pow_spectrum;
	pow_spectrum.data = malloc(sizeof(SAMPLE) * frame.row * (pointFFT / 2 + 1));

	for (int i = 0; i < frame.row; i++) {
		for (int k = 0; k < pointFFT / 2 + 1; k++)
		{
			for (int n = 0; n < frame.col; n++)
			{
				float term = -2 * PI * (k + 1) * (n + 1) / frame.col;
				temp = frame.data[i * frame.col + n] * HammingWindow(n, frame.col);
				real += temp * cos(term);
				img += temp * sin(term);
				//if (n > 320) {
				//	printf("");
				//}
			}
			temp = magnitude(real, img);

			pow_spectrum.data[i* (pointFFT / 2 + 1) + k] = temp * temp / frame.col;
			//if (i* (pointFFT / 2 + 1) + k > 31619) {

			//	printf("%.2f", pow_spectrum.data[i* (pointFFT / 2 + 1) + k]);
			//}
			real = img = 0;
		}

	}
	//de-allocate
	free(frame.data);

	pow_spectrum.col = pointFFT / 2 + 1;
	pow_spectrum.row = frame.row;

	return pow_spectrum;
}

float magnitude(float real, float img)
{
	return sqrt(real*real + img * img);
}

filter_bank filterbank(int nfilt, int NFFT)
{
	int lowfreq_mel = 0;                    //c?n du?i thang Mel
	float highfreq_mel = (float)hz2mel(SAMPLE_RATE / 2);   //C?n trên
	float *melpoint = (float*)malloc(sizeof(float)*(nfilt + 2));
	float *hzpoint = (float*)malloc(sizeof(float)*(nfilt + 2));
	float *bin = (float*)malloc(sizeof(float)*(nfilt + 2));           //FFT bins du?c tính theo công th?c (NFFT + 1) * hzpoints / SAMPLE_RATE

	float step = (highfreq_mel - lowfreq_mel) / (nfilt + 2);       //bu?c chuy?n tuy?n tính thang Mel
	for (int i = 0; i < nfilt + 2; i++)
	{
		if (i == 0)
		{
			melpoint[i] = lowfreq_mel;
			hzpoint[i] = (float)mel2hz(melpoint[i]);
			bin[i] = floor(((NFFT + 1) * (float)hzpoint[i]) / SAMPLE_RATE);
			continue;
		}
		if (i == nfilt + 1)
		{
			melpoint[i] = highfreq_mel;
			hzpoint[i] = SAMPLE_RATE / 2;
			bin[i] = floor(((NFFT + 1) * (float)hzpoint[i]) / SAMPLE_RATE);
			break;
		}
		melpoint[i] = melpoint[i - 1] + step;
		hzpoint[i] = (float)mel2hz(melpoint[i]);
		bin[i] = floor(((NFFT + 1) * (float)hzpoint[i]) / SAMPLE_RATE);
	}
	free(melpoint);
	free(hzpoint);
	int a = (int)floor((1.0 * NFFT / 2) + 1);
	float* fbank = (float*)calloc(nfilt* a, sizeof(float));       //26 filter, moi filter chi co 1 diem co gia tri bang 1 (chinh la tan so dc chia theo thang tuyen tinh)           


																  //tính filterbanks theo công th?c.
	for (int m = 1; m < nfilt + 1; m++)
	{
		int f_m_minus_1 = (int)bin[m - 1];      //di?m b?t d?u
		int f_m = (int)bin[m];                  //d?t c?c tr?
		int f_m_plus_1 = (int)bin[m + 1];       //k?t thúc c?a 1 filter
		for (int k = f_m_minus_1; k < f_m; k++)     //len dan cho den khi dat cuc dai fbank (0 - 1)
		{
			fbank[(m - 1)* a + k] = (k - bin[m - 1]) / (bin[m] - bin[m - 1]);     //chia nguyen -> chi bang 1 tai diem f_m (1 so tan so xac dinh)
		}
		for (int k = f_m; k < f_m_plus_1; k++)      //chu y de toi uu
		{
			fbank[(m - 1)* a + k] = (bin[m + 1] - k) / (bin[m + 1] - bin[m]);
		}
	}
	free(bin);
	return getFBank(fbank, nfilt, a);
}

float HammingWindow(float a, int frameLength)
{
	return 0.54 - 0.46 * cos(2 * PI * a / (frameLength - 1));
}

int getLength(SAMPLE * a)
{
	return sizeof(a) / sizeof(a[0]);
}

hyper_vector make_hyper_vector(int row, int col, int dim)
{
	hyper_vector vector;
	vector.row = row;
	vector.col = col;
	vector.dim = dim;
	vector.data = (float *)malloc(sizeof(float) * row * col * dim);
	return vector;
}

void free_hyper_vector(hyper_vector vector)
{
	free(vector.data);
}

filter_bank getFBank(float *fbank, int nfilt, int filt_len) {
	filter_bank temp;
	temp.data = (float *)malloc(sizeof(float) * nfilt * filt_len);
	for (int i = 0; i < nfilt * filt_len; i++) {
		temp.data[i] = fbank[i];
	}
	temp.nfilt = nfilt;
	temp.filt_len = filt_len;
	free(fbank);
	return temp;
}


SIGNAL setSignal(SAMPLE * a, int size)
{
	SIGNAL temp;
	temp.raw_signal = (float *)malloc(sizeof(float) * size);
	for (int i = 0; i < size; ++i) {
		temp.raw_signal[i] = a[i];
	}
	temp.frame_length = SAMPLE_RATE * 0.025;
	temp.step_lengh = SAMPLE_RATE * 0.01;
	temp.signal_length = size;
	free(a);
	return temp;
}

hyper_vector setHVector(SAMPLE * a, int col, int row, int dim)
{
	hyper_vector temp_vector;
	temp_vector.col = col;
	temp_vector.row = row;
	temp_vector.dim = dim;
	temp_vector.data = (SAMPLE *)malloc(sizeof(SAMPLE) * row * col *dim);
	for (int i = 0; i < col * row * dim; ++i) {
		temp_vector.data[i] = a[i];
	}
	free(a);
	return temp_vector;
}

hyper_vector getFrames(SIGNAL a)
{
	int signal_len = a.signal_length;
	int frame_len = a.frame_length;
	int frame_step = a.step_lengh;

	if (signal_len <= frame_len)        //s? m?u toàn tín hi?u nh? hon d? r?ng khung
		a.num_frame = 1;
	else
	{
		float num_additional_frames = (float)(signal_len - frame_len) / frame_step;   //so frame khong tinh frame cuoi
		num_additional_frames = (ceil(num_additional_frames));
		a.num_frame = 1 + (int)num_additional_frames;                             //numframes (frame cuoi day du hoac khong)
	}

	int padsignal_len = (a.num_frame - 1) * frame_step + frame_len;      //do dai chuoi tin hieu neu cac frame day du
	int zeros = padsignal_len - signal_len;                //Do dai chuoi Zeros can pad.

	SAMPLE *signal = (float*)malloc(sizeof(float)*padsignal_len);

	//realloc(a.raw_signal, padsignal_len);
	for (int i = 0; i < padsignal_len; i++)         //chen them 0 vao frame cuoi.
	{
		if (i < signal_len) {
			signal[i] = a.raw_signal[i];
			continue;
		}
		signal[i] = 0;
	}

	// thuc hien chia frame (0:0->framelen, 1:framestep->(framelen + framestep),...
	int index = 0;
	int dem1 = 0, dem2 = 0;
	int temp = frame_step;

	SAMPLE *frames = (SAMPLE*)malloc(sizeof(SAMPLE) * a.num_frame * a.frame_length);

	for (int i = 0; i < a.num_frame; i++) {
		for (int j = 0; j < frame_len; j++)
			frames[i * frame_len + j] = 0;
	}
	/*system("cls");
	float temp12;*/
	while (index < a.num_frame)
	{
		if (index == 0)                 //frame dau tien
			for (int i = 0; i < frame_len; i++)
			{
				frames[index * frame_len + i] = signal[i];

				/*printf("%f  ", frames[index * frame_len + i]);*/
			}
		else                          //cac frames con lai, framestep->(framelen + framestep)...
		{
			for (int i = temp; i < temp + frame_len; i++)
			{
				if (index * frame_len + dem1 > 49538) {
					printf("");
				}
				frames[index * frame_len + dem1++] = signal[i];


			}
			temp += frame_step;
			dem1 = 0;
		}
		index++;
	}


	return setHVector(frames, frame_len, a.num_frame, 1);
}

void append_energy(hyper_vector dct, hyper_vector pow_spec)
{
	float sum;
	for (int i = 0; i < pow_spec.row; i++) {
		sum = 0;
		for (int j = 0; j < pow_spec.col; j++) {
			sum += pow_spec.data[i*pow_spec.col + j];
		}
		dct.data[i*dct.col] = 20 * log10(sum);
	}
}

SIGNAL preEmphasis(SAMPLE *a, int size, float preemh) {
	SIGNAL temp;
	temp.raw_signal = (float *)malloc(sizeof(float) * size);
	for (int i = 0; i < size; ++i) {
		if (i == 0) {
			temp.raw_signal[i] = a[i];
			continue;
		}

		temp.raw_signal[i] = a[i] - preemh * a[i - 1];
		/*if (i > 48500) {
		printf("%f ", temp.raw_signal[i]);
		}*/
		/*if (i>20015 && i<20030)
		printf(" %f ", temp.raw_signal[i]);
		*/
	}
	temp.frame_length = SAMPLE_RATE * 0.025;
	temp.step_lengh = SAMPLE_RATE * 0.01;
	temp.signal_length = size;
	return temp;
}

void write_feature_vector_to_database(hyper_vector feature_vector, char *name)
{
	char *path = (char *)"./feature_vector/";
	size_t len = strlen(name);
	char *absolute_path = (char *)malloc(sizeof(char) * (len + 18));
	strcpy(absolute_path, path);
	//strcat();
}

int check_path(char * path)
{
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "file no exist!!! \n");
		return 1;
	}
	fclose(fp);
	return 0;
}

void writeDBFS(SAMPLE* raw_signal, int trim_ms, int signal_len) {

	FILE *f = fopen("./data/file.txt", "w");
	if (f == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}
	for (int i = 0; i < signal_len; i++) {
		if (i < 360) {
			raw_signal[i] = 0;
			fprintf(f, "%.8f\n", raw_signal);
			continue;
		}
		fprintf(f, "%.8f\n", raw_signal[i]);
	}
	fclose(f);

	FILE *f2 = fopen("./data/file2.txt", "w");
	int chunk_size = 160;
	float sum = 0;
	while (trim_ms < signal_len)
	{
		sum = 0;
		for (int i = trim_ms; i < trim_ms + chunk_size; i++) {
			sum += raw_signal[i] * raw_signal[i];
		}
		sum = sqrt(sum / (chunk_size));
		sum = 20 * log10(sum);
		fprintf(f2, "%f\n", sum);
		trim_ms += chunk_size;
	}
	fclose(f2);
}


hyper_vector get_feature_vector_from_signal(SAMPLE * audio_signal, int size)
{
	/*______________________get_pre_emphasized_signal_________________________________________________*/
	SIGNAL a = setSignal(audio_signal, size);

	/*______________________get_silence_free_signal_________________________________________________*/
	writeDBFS(a.raw_signal, 0, size);
	SIGNAL temp = silence_trim(a);
	free(a.raw_signal);
	/*______________________get_Frames________________________________________________________________*/
	hyper_vector frames = getFrames(temp);
	free(temp.raw_signal);
	/*______________________compute_DFT_and_Power_spectrum____________________________________________*/
	hyper_vector power_spec = DFT_PowerSpectrum(frames, 512);
	/*______________________get_filterbanks___________________________________________________________*/
	filter_bank fbanks = filterbank(26, 512);
	/*______________________apply_filterBanks_________________________________________________________*/
	hyper_vector transpose_param = setHVector(fbanks.data, fbanks.filt_len, fbanks.nfilt, 1);
	hyper_vector tmp = transpose(transpose_param);
	free(transpose_param.data);
	hyper_vector apply = multiply(power_spec, tmp);
	free(tmp.data);
	/*______________________get_more_compact_output_by_performing_DCT_conversion_______________________*/
	hyper_vector test = DCT(apply, 13);
	free(apply.data);
	/*______________________append_frame_energy_into_mfcc_vectors______________________________________*/
	append_energy(test, power_spec);
	free(power_spec.data);
	/*______________________final_feature_vector_size_1x91_____________________________________________*/
	hyper_vector final_feats = cov(test);
	free(test.data);
	return final_feats;
}

hyper_vector get_first_single_frame(hyper_vector feature_vector)
{
	hyper_vector first_single_frame = make_hyper_vector(feature_vector.col, 1, 1);
	for (int i = 0; i < feature_vector.col; ++i) {
		first_single_frame.data[i] = feature_vector.data[i];
	}
	return first_single_frame;
}