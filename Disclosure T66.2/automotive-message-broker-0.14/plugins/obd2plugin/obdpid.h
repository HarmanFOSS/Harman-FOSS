#ifndef _OBDPID_H__H_H_
#define _OBDPID_H__H_H_

#include <vector>
#include <string>
#include <vehicleproperty.h>
#include "obdlib.h"
#include <time.h>

class ObdPid
{
public:
	typedef std::vector<unsigned char> ByteArray;

	ObdPid(VehicleProperty::Property prop, std::string p, int i)
		:property(prop), pid(p), id(i), type(0x41)
	{
		isValidVal = false;
	}
	static ByteArray cleanup(ByteArray replyVector)
	{
		ByteArray tmp;

		for (int i=0;i<replyVector.size();i++)
		{
			if ((replyVector[i] != 0x20) && (replyVector[i] != '\r') && (replyVector[i] != '\n'))
			{
				tmp.push_back(replyVector[i]);
			}
		}
		return tmp;
	}
	static ByteArray compress(ByteArray replyVector)
	{
		ByteArray tmp;
		for (int i=0;i<replyVector.size()-1;i++)
		{
			tmp.push_back(obdLib::byteArrayToByte(replyVector[i], replyVector[i+1]));
			i++;
		}
		return tmp;
	}
	virtual ObdPid* create() = 0;

	bool tryParse(ByteArray replyVector)
	{
		if (!isValid(replyVector))
		{
			return false;
		}
		parse(replyVector);
		return true;
	}
	virtual void parse(const ByteArray &replyVector) = 0;
	virtual bool isValid(const ByteArray &replyVector) = 0;
	virtual bool needsCompress() { return true; }

	VehicleProperty::Property property;
	std::string pid;
	int id;
	int type;
	std::string value;
	bool isValidVal;
};

template <class T>
class CopyMe: public ObdPid
{
public:

	CopyMe(VehicleProperty::Property prop, std::string p, int i)
		:ObdPid(prop, p, i)
	{
	}

	ObdPid* create()
	{
		T* t = new T();
		t->value = value;
		t->isValidVal = isValidVal;
		return t;
	}
};


class VehicleSpeedPid: public CopyMe<VehicleSpeedPid>
{
public:

	VehicleSpeedPid()
		:CopyMe(VehicleProperty::VehicleSpeed, "010D1\r", 0x0D)
	{

	}
	bool isValid(const ByteArray &replyVector)
	{
		isValidVal = true;
		if (replyVector[1] != id)
		{
			return isValidVal = false;
		}

		return isValidVal;
	}
	void parse(const ByteArray &replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}

		int mph = replyVector[2];
		value = boost::lexical_cast<std::string>(mph);
	}
};

class EngineSpeedPid: public CopyMe<EngineSpeedPid>
{
public:

	EngineSpeedPid()
		:CopyMe(VehicleProperty::EngineSpeed,"010C1\r",0x0C)
	{

	}
	bool isValid(const ByteArray &replyVector)
	{
		if (replyVector[1] != id)
		{
			isValidVal = false;
			return false;
		}
		isValidVal = true;
		return true;
	}
	void parse(const ByteArray &replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		double rpm = ((replyVector[2] << 8) + replyVector[3]) / 4.0;
		value = boost::lexical_cast<std::string>(rpm);
	}
};

class EngineCoolantPid: public CopyMe<EngineCoolantPid>
{
public:

	EngineCoolantPid()
		:CopyMe(VehicleProperty::EngineCoolantTemperature,"01051\r",0x05)
	{

	}
	bool isValid(const ByteArray &replyVector)
	{
		if (replyVector[1] != id)
		{
			isValidVal = false;
			return false;
		}
		isValidVal = true;
		return true;
	}
	void parse(const ByteArray &replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		int temp = replyVector[2] - 40;
		value = boost::lexical_cast<std::string>(temp);
	}
};

class MassAirFlowPid: public CopyMe<MassAirFlowPid>
{
public:

	MassAirFlowPid()
		:CopyMe(VehicleProperty::MassAirFlow,"01101\r", 0x10)
	{

	}
	bool isValid(const ByteArray &replyVector)
	{
		if (replyVector[1] != id)
		{
			isValidVal = false;
			return false;
		}
		isValidVal = true;
		return true;
	}
	void parse(const ByteArray &replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}

		maf = ((replyVector[2] << 8) + replyVector[3]) / 100.0;
		value = boost::lexical_cast<std::string>(maf);
	}

protected:
	double maf;
};


class FuelConsumptionPid: public MassAirFlowPid
{
public:
	FuelConsumptionPid()

	{

	}
	bool isValid(const ByteArray &replyVector)
	{
		return isValidVal = MassAirFlowPid::isValid(replyVector);
	}
	void parse(const ByteArray & replyVector)
	{
	  if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}

		timespec t;
		clock_gettime(CLOCK_REALTIME, &t);

		double currentTime = t.tv_sec + t.tv_nsec / 1000000;

		double diffTime = currentTime - oldTime;
		oldTime = currentTime;

		double consumption = 1 / (14.75 * 6.26) * maf * diffTime/60;

		value = boost::lexical_cast<std::string>(consumption);
	}

private:

	static double oldTime;
};


class VinPid: public CopyMe<VinPid>
{
public:

	VinPid()
		:CopyMe(VehicleProperty::VIN,"0902\r",0x02)
	{
		type = 0x49;
	}
	bool isValid(const ByteArray & replyVector)
	{
		isValidVal = true;

		if (replyVector[0] != 0x49 || replyVector[1] != 0x02)
		{
			isValidVal = false;

		}
		return isValidVal;
	}
	void parse(const ByteArray & replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		std::string vinstring;
		for (int j=0;j<replyVector.size();j++)
		{
			if(replyVector[j] == 0x49 && replyVector[j+1] == 0x02)
			{
				//We're at a reply header
				j+=3;
			}
			if (replyVector[j] != 0x00)
			{
				vinstring += (char)replyVector[j];
				//printf("VIN: %i %c\n",replyVector[j],replyVector[j]);
			}
		}

		value = vinstring;
	}
};

class WmiPid: public VinPid
{
public:

	WmiPid()
		:VinPid()
	{
		property = VehicleProperty::WMI;
	}
	bool isValid(const ByteArray & replyVector)
	{
		return isValidVal = VinPid::isValid(replyVector);
	}
	void parse(const ByteArray &replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		VinPid::parse(replyVector);
		value = value.substr(0,3);
	}
};

class AirIntakeTemperaturePid: public CopyMe<AirIntakeTemperaturePid>
{
public:
	AirIntakeTemperaturePid()
		:CopyMe(VehicleProperty::AirIntakeTemperature,"010F1\r",0x0F)
	{

	}
	bool isValid(const ByteArray & replyVector)
	{
		if (replyVector[1] != id)
		{
			isValidVal = false;
			return false;
		}
		isValidVal = true;
		return true;
	}
	void parse(const ByteArray & replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		int temp = replyVector[2] - 40;
		value = boost::lexical_cast<std::string>(temp);
	}
};

class EngineLoadPid: public CopyMe<EngineCoolantPid>
{
public:
	EngineLoadPid()
		:CopyMe(VehicleProperty::EngineLoad,"01041\r",0x04)
	{

	}
	bool isValid(const ByteArray & replyVector)
	{
		if (replyVector[1] != id)
		{
			isValidVal = false;
			return false;
		}
		isValidVal = true;
		return true;
	}
	void parse(const ByteArray &replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		int load = replyVector[2]*100.0/255.0;
		value = boost::lexical_cast<std::string>(load);
	}
};

class ThrottlePositionPid: public CopyMe<ThrottlePositionPid>
{
public:
	ThrottlePositionPid()
		:CopyMe(VehicleProperty::ThrottlePosition,"01111\r",0x11)
	{

	}
	bool isValid(const ByteArray & replyVector)
	{
		if (replyVector[1] != id)
		{
			isValidVal = false;
			return false;
		}
		isValidVal = true;
		return true;
	}
	void parse(const ByteArray & replyVector)
	{
		if (!isValidVal)
		{
			//TODO: Determine if we should throw an exception here, rather than just returning without a value?
			return;
		}
		int temp = replyVector[2]*100.0/255.0;
		value = boost::lexical_cast<std::string>(temp);
	}
};

class BatteryVoltagePid: public CopyMe<BatteryVoltagePid>
{
public:
	BatteryVoltagePid()
		:CopyMe(VehicleProperty::BatteryVoltage,"ATRV\r",0)
	{

	}

	bool needsCompress() { return false; }

	bool isValid(const ByteArray & replyVector)
	{
		if(replyVector[replyVector.size() - 1] == 'V')
		{
			return isValidVal = true;
		}
		return false;
	}

	void parse(const ByteArray & replyVector)
	{
		value = "";
		for(int i=0; i<replyVector.size() - 1; i++)
		{
			value += replyVector[i];
		}
	}

};

#endif
