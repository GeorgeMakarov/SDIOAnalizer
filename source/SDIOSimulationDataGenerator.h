#ifndef SDIO_SIMULATION_DATA_GENERATOR
#define SDIO_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class SDIOAnalyzerSettings;

class SDIOSimulationDataGenerator
{
public:
	SDIOSimulationDataGenerator();
	~SDIOSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, SDIOAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	SDIOAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
    void SDIOaddUINT(U32 value, int bits);
    void SDIOclockIt(void);
protected:
	void CreateSDIO();

	std::string mSDIOText;
	U32 mCmdIndex;
    U32 samples_per_bit;

    SimulationChannelDescriptor mSDIOSimulationCmd;
    SimulationChannelDescriptor mSDIOSimulationClk;
    SimulationChannelDescriptor mSDIOSimulationDat;

};
#endif //SDIO_SIMULATION_DATA_GENERATOR
