

#ifndef PROGC_SRC_PROCESSORS_PROCESSOR_H
#define PROGC_SRC_PROCESSORS_PROCESSOR_H

class Processor
{
public:

	virtual void process() = 0;

	virtual ~Processor() = default;
};

#endif //PROGC_SRC_PROCESSORS_PROCESSOR_H
