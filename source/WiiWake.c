#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <network.h>
#include <gccore.h>

#include <wiiuse/wpad.h>

int udp_broadcast(u16 port, char *message, u32 length) {
	
	while(net_init() < 0);
	
	struct sockaddr_in address;
	
	s32 result;
	int optval = 1;
	s32 socket;
	
	memset(&address, 0, sizeof(struct sockaddr_in));
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_BROADCAST;
	address.sin_port = htons(port);
	address.sin_len = sizeof(struct sockaddr);
	
	if((socket = net_socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
		fprintf(stderr, "Failed to build socket.");
		return -1;
	}
	
	if((result = net_ioctl(socket, FIONBIO, (char *)&optval)) < 0) {
		fprintf(stderr, "Failed to set non-blocking IO mode (%d).", result);
		return -1;
	}
	
	if((result = net_setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval))) < 0) {
		fprintf(stderr, "Failed to set broadcasting mode (%d).", result);
		return -1;
	}
	
	if((result = net_sendto(socket, message, length, 0, (struct sockaddr *)&address, address.sin_len)) < 0) {
		fprintf(stderr, "Failed to broadcast message (%d).", result);
		return -1;
	}
	
	net_close(socket);
	
	return 0;
}

int udp_wake(u8 *ethaddr, u16 port) {
	
	const u8 rows = 6;
	const u8 columns = 17;
	
	char message[102] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	
	u8 r = 1; // Skip first row, its already written
	while(r < rows) {
		u8 c = 0;
		while(c < columns) {
			u16 index = r * columns + c;
			message[index] = ethaddr[c];
			c++;
		}
		r++;
	}
	
	return udp_broadcast(port, message, rows * columns);
}

int main(int argc, char **argv) {
	
	VIDEO_Init();
	WPAD_Init();
	
	GXRModeObj *video_mode = VIDEO_GetPreferredMode(NULL);
	void *video_framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(video_mode));
	
	console_init(video_framebuffer, 20, 20, video_mode->fbWidth, video_mode->xfbHeight, video_mode->fbWidth * VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(video_mode);
	VIDEO_SetNextFramebuffer(video_framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	
	if(video_mode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
	
	printf("\x1b[2;0H");
	
	u8 ethaddr[] = { 0x48, 0x5B, 0x39, 0x05, 0x52, 0x08 }; // Target computer MAC address
	
	if (udp_wake(ethaddr, 9) < 0) {
		printf("Failed!\n");
	}
	else {
		printf("Success!\n");
	}
	
	while(1) {
		
		VIDEO_WaitVSync();
		
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0)) exit(0);
	}
	
	return 0;
}
