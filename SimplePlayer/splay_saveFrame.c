#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include <stdio.h>
#include <math.h>

void SaveFrame(AVFrame *pFrame,int width,int height,int iFrame)
{
	FILE *pFile;
	char szFilename[32];
	int y;

	sprintf(szFilename,"frame%d.ppm",iFrame);
	pFile = fopen(szFilename,"wb");
	if(pFile == NULL)
	{
		return;
	}

	//Write header

	fprintf(pFile,"P6\n%d %d\n255\n",width,height);

	//Write pixel data
	for(y=0; y<height;y++)
	{
		fwrite(pFrame->data[0]+y*pFrame->linesize[0],1,width*3,pFile);
	}

	fclose(pFile);
}

int main(int argc,char *argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	int				i,videoStream;
	AVCodecContext 	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame			*pFrame;
	AVFrame			*pFrameRGB;
	AVPacket		packet;
	int 			frameFinished;
	int				numBytes;
	uint8_t			*buffer;

	if(argc < 2)
	{
		printf("Please provide a movie file\n");
		return -1;
	}

	av_register_all();
    
	// if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL))
	if(avformat_open_input(&pFormatCtx,argv[1],NULL,NULL)!=0)
	{
		return -1;
	}

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx ,NULL) < 0)
		return -1;

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx,0,argv[1],0);

	// Find the first video stream
	for(i=0; i<pFormatCtx->nb_streams;i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			break;
		}
	}

	if(videoStream == -1)
	{
		return -1;//Didn't find a video stream
	}

	// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec == NULL)
	{
		printf("Unsupported codec!\n");
		return -1;
	}

	// Open codec
	if(avcodec_open(pCodecCtx,pCodec) < 0)
		return -1;//Could not open codec

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	pFrameRGB = avcodec_alloc_frame();
	if(pFrameRGB == NULL)
		return -1;

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGB24,pCodecCtx->width, pCodecCtx->height);
	buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset 
	// of AVPicture 
	avpicture_fill((AVPicture *)pFrameRGB,buffer,PIX_FMT_RGB24,pCodecCtx->width,
					pCodecCtx->height);

	static struct SwsContext *img_convert_ctx;
	if(img_convert_ctx == NULL)
	{
		img_convert_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height
										, pCodecCtx->pix_fmt,
										pCodecCtx->width, pCodecCtx->height,
										PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
		if(img_convert_ctx == NULL)
		{
			 printf("Cannot initialize the conversion context\n");
			 return 0;
		}
	}

	//Read frames and save first five frames to disk
	i = 0;
	while(av_read_frame(pFormatCtx, &packet) >=0)
	{
		// Is this a packet from the video stream?
        printf("read frame done\n");
		if(packet.stream_index == videoStream)
		{
			
			while(packet.size > 0)
			{
				int len = avcodec_decode_video2(pCodecCtx,pFrame,&frameFinished,&packet);
				if(len > 0)
				{
					printf("decode successfule\n");
		
					if(frameFinished&&(pFrame->key_frame == 1))
					{
						sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
						if(++i<=5)
							SaveFrame(pFrameRGB,pCodecCtx->width,pCodecCtx->height,i);
					}
				}
				packet.size = packet.size - len;
				packet.data = packet.data + len;
			}
			
		}

		if(i>5)
			break;
	}
	//av_free_packet(&packet);

	av_free(buffer); 
	av_free(pFrameRGB);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}

