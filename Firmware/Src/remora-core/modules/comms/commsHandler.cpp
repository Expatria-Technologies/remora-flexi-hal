#include "commsHandler.h"
#include "../../remora.h"

CommsHandler::CommsHandler() : data(false), noDataCount(0), status(false) {

}

CommsHandler::~CommsHandler() {}

void CommsHandler::init() {
    interface->setDataCallback([this](bool dataReceived) {
        this->setData(dataReceived);
    });

    interface->init();
}

void CommsHandler::start() {
	interface->start();
}

// tasks is run in the main loop to do polling tasks, eg Ethernet, processing data etc
void CommsHandler::tasks() {
	interface->tasks();
}

// update it run in the servo thread to monitor communications
void CommsHandler::update() {
	if (data)
	{
		noDataCount = 0;
		status = true;
	}
	else
	{
		noDataCount++;
	}

	if (noDataCount > Config::dataErrMax)
	{
		noDataCount = 0;
		status = false;
	}

	data = false;
}
