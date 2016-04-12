#include <header/SensorServer.h>

SensorServer::SensorServer() : mOrientation(new float[3]) {
    mServerThread = thread(&SensorServer::makeConnection, this);
}

void SensorServer::makeConnection() {
	if ( (mSocketFD = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) {
        logMsg(LOG_ERROR, "Establish socket failed", 3);
        exit(-1);
    }

    bzero ( (char *)&mServerAddr, sizeof(mServerAddr) );
    mServerAddr.sin_family = AF_INET;
    mServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mServerAddr.sin_port = htons(SERV_PORT);

    if ( bind(mSocketFD, (struct sockaddr *)&mServerAddr, sizeof(mServerAddr) ) < 0) {
        logMsg(LOG_ERROR, "Bind socket failed", 3);
        exit(-2);
    }

    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    int readBytes;

    char* buf = new char[BUF_SIZE];

    logMsg(LOG_INFO, "Sensor connection loop START!!", 3);

    while (true) {
        readBytes = recvfrom(mSocketFD, buf, BUF_SIZE, 0, (struct sockaddr*)&clientAddr, (socklen_t *)&clientAddrSize) ;

        if (readBytes <= 0)
            break;
        parseSensorInfo(buf);
    }
    logMsg(LOG_INFO, "Sensor connection loop END!!", 3);    

    delete[] buf;
}

void SensorServer::parseSensorInfo(char* buf) {
    //vector<string> ans;
    char *src = strdup(buf);
    char *ptr = strtok(src, ","); 
    int counter = 0;
    while (ptr != NULL && counter < 3) {
        mOrientation[counter] = atof(ptr);
        counter++;
        ptr = strtok(NULL, ","); 
    }
    //logMsg(LOG_DEBUG, stringFormat("Orientation Updated: %f, %f, %f", mOrientation[0], mOrientation[1], mOrientation[2]));
}

float* SensorServer::getClientOrientation() {
    return mOrientation;
}