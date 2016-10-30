#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <string>
#include <sstream>

#include "opencv2/opencv.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace cv;
using namespace std;


/** Global variables */
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

void recordCount(string imageName, int count){

    char filename[]="HeadCount.txt";
    fstream fp;

    fp.open(filename, ios::out|ios::app);
    fp << imageName.substr(0,19) << " " << count << endl;
    fp.close();
}

int faceDetection( string imageName )
{
    //-- 1. Load the cascades
    if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading face cascade\n"); return -1; };
    if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading eyes cascade\n"); return -1; };

    Mat frame;
    frame = imread( imageName, 1 );
    //cout << "Face detection: " << imageName << endl ;

    std::vector<Rect> faces;
    Mat frame_gray;

    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );

    //-- Detect faces
    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30) );
    printf("Head counter: %d\n",faces.size());
    return faces.size();
}

int receive_image(int socket)
{	// Start function

	int buffersize = 0, recv_size = 0, size = 0, read_size, write_size, packet_index = 1, stat;

	char imagearray[10241], verify = '1';
	FILE *image;
	char name[128];

//Find the name of the image
	do {
		bzero(name, 128);
		stat = read(socket, name, sizeof(name));
	} while (stat < 0);

//Find the size of the image
	do {
		stat = read(socket, &size, sizeof(int));
	} while (stat < 0);

	printf("Packet received.\n");
	printf("Packet size: %i\n", stat);
	cout << "Image name: " << name << endl;
	printf("Image size: %i\n", size);

	char buffer[] = "Got it";

//Send our verification signal
	do {
		stat = write(socket, &buffer, sizeof(int));
	} while (stat < 0);

	image = fopen(name, "w");

	if ( image == NULL) {
		printf("Error has occurred. Image file could not be opened\n");
		return -1;
	}

//Loop while we have not received the entire file yet


	int need_exit = 0;
	struct timeval timeout = {10, 0};

	fd_set fds;
	int buffer_fd, buffer_out;

	while (recv_size < size) {
//while(packet_index < 2){

		FD_ZERO(&fds);
		FD_SET(socket, &fds);

		buffer_fd = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

		if (buffer_fd < 0)
			printf("error: bad file descriptor set.\n");

		if (buffer_fd == 0)
			printf("error: buffer read timeout expired.\n");

		if (buffer_fd > 0)
		{
			do {
				read_size = read(socket, imagearray, 10241);
			} while (read_size < 0);

			//printf("Packet number received: %i\n", packet_index);
			//printf("Packet size: %i\n", read_size);


			//Write the currently read data into our image file
			write_size = fwrite(imagearray, 1, read_size, image);
			//printf("Written image size: %i\n", write_size);

			if (read_size != write_size) {
				printf("error in read write\n");
			}


			//Increment the total number of bytes read
			recv_size += read_size;
			packet_index++;
			//printf("Total received image size: %i\n", recv_size);
			//printf(" \n");
			//printf(" \n");
		}

	}


	fclose(image);
	printf("Image successfully Received!\n");

	int count = faceDetection(name);
	printf("MQTT publish!\n");
	
	
	std::ostringstream cmd;
	cmd << "mosquitto_pub -h " << getenv("MQTT_HOST") << " -d -t '" << getenv("MQTT_TOPIC") <<"' -m '" << count << "'"; 
	system(cmd.str().c_str()); 

	//recordCount(name, count);
	remove(name);
	printf("Delete image\n");

	return 1;
}


int main(int argc , char *argv[])
{

	int socket_desc;
	struct sockaddr_in server;
	char *parray;


	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);

	if (socket_desc == -1) {
		printf("Could not create socket");
	}

	memset(&server, 0, sizeof(server));
	cout << "MODULE1_HOST: " << getenv("MODULE1_HOST");
	cout << "MODULE1_PORT: " << getenv("MODULE1_PORT");
	server.sin_addr.s_addr = inet_addr( getenv("MODULE1_HOST") );
	server.sin_family = AF_INET;
	server.sin_port = htons( atoi(getenv("MODULE1_PORT")) );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0) {
		cout << strerror(errno);
		close(socket_desc);
		puts("Connect Error");
		return 1;
	}

	puts("Connected");

	receive_image(socket_desc);

	close(socket_desc);

	return 0;
}

