#ifndef COMMSHANDLER_H
#define COMMSHANDLER_H

#include <cstdint>
#include <memory>

#include "../../modules/module.h"
#include "../../comms/commsInterface.h"

class CommsHandler : public Module {
private:
	std::unique_ptr<CommsInterface> interface;
	bool data;
	uint8_t noDataCount;
	bool status;

public:
	CommsHandler();
	virtual ~CommsHandler();

	void update(void) override;
	void tasks();

    void setInterface(std::unique_ptr<CommsInterface> commsInterface) {
        interface = std::move(commsInterface);
    }

    void init();
    void start();

    void setData(bool value) { data = value; }
    void setNoDataCount(int count) { noDataCount = count; }
    void setStatus(bool value) { status = value; }

    bool getData() const { return data; }
    int getNoDataCount() const { return noDataCount; }
    bool getStatus() const { return status; }
};

#endif
