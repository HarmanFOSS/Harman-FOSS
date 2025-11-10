import sys

#filename = r"D:\Helmut__Comp\RPi\rpi_positioning_logs\20151025__Gartenamt_Hu\20151025__Gartenamt_Hu.txt"


#TODOexception handling in parsing

#FORMAT timestamp,countdown,$GVGNSTIM,timestamp,year,month,day,hour,minute,second,ms,validityBits
class GVGNSTIM:
    """Encapsulate a GVGNSTIM sentence.
    
    The attributes correspond to the fields in the corresponding
    C data structure TGNSSTime from header file gnss.h
    Invalid data are set to None
    """

    GNSS_TIME_TIME_VALID    = 0x00000001   #Validity bit for field TGNSSTime fields hour, minute, second, ms.
    GNSS_TIME_DATE_VALID    = 0x00000002   #Validity bit for field TGNSSTime fields year, month, day.
    GNSS_TIME_SCALE_VALID   = 0x00000004   #Validity bit for field TGNSSTime field scale.
    GNSS_TIME_LEAPSEC_VALID = 0x00000008   #Validity bit for field TGNSSTime field leapSeconds.

    def __init__(self):
        self.valid = False
        self.timestamp = None
        self.year = None
        self.month = None
        self.day = None
        self.hour = None
        self.minute = None
        self.second = None
        self.ms = None
        self.scale = None
        self.leapSeconds = None
        
    def from_logstring(self, logstring):
        self.__init__()
        logstring = logstring.rstrip() #remove \r\n at end of string
        fields = logstring.split(",")
        if (len(fields) == 14) and (fields[2] == "$GVGNSTIM"):
            #print(logstring)
            #print(fields)
            self.timestamp = int(fields[3])
            self.validity_bits = int(fields[13],16)
            if self.validity_bits & GVGNSTIM.GNSS_TIME_DATE_VALID:
                self.year = int(fields[4])
                self.month = int(fields[5])
                self.day = int(fields[6])
            if self.validity_bits & GVGNSTIM.GNSS_TIME_TIME_VALID:
                self.hour = int(fields[7])
                self.minute = int(fields[8])
                self.second = int(fields[9])
                self.ms = int(fields[10])
            if self.validity_bits & GVGNSTIM.GNSS_TIME_SCALE_VALID:
                self.scale = int(fields[11])
            if self.validity_bits & GVGNSTIM.GNSS_TIME_LEAPSEC_VALID:
                self.leapSeconds = int(fields[12])                
            self.valid = True
        return self.valid

#FORMAT timestamp,countdown,$GVGNSPOS,timestamp,latitude,longitude,altitudeMSL,altitudeEll,hSpeed,vSpeed,  heading,pdop,hdop,vdop,usedSatellites,trackedSatellites,visibleSatellites,sigmaHPosition,sigmaAltitude,sigmaHSpeed,sigmaVSpeed,sigmaHeading,fixStatus,fixTypeBits,activated_systems,used_systems,validityBits
class GVGNSPOS:
    """Encapsulate a GVGNSPOS sentence.
    
    The attributes correspond to the fields in the corresponding
    C data structure TGNSSPosition from header file gnss.h
    Invalid data are set to None
    """

    GNSS_POSITION_LATITUDE_VALID        = 0x00000001    # Validity bit for field TGNSSPosition::latitude. 
    GNSS_POSITION_LONGITUDE_VALID       = 0x00000002    # Validity bit for field TGNSSPosition::longitude. 
    GNSS_POSITION_ALTITUDEMSL_VALID     = 0x00000004    # Validity bit for field TGNSSPosition::altitudeMSL. 
    GNSS_POSITION_ALTITUDEELL_VALID     = 0x00000008    # Validity bit for field TGNSSPosition::altitudeEll. 
    GNSS_POSITION_HSPEED_VALID          = 0x00000010    # Validity bit for field TGNSSPosition::hSpeed. 
    GNSS_POSITION_VSPEED_VALID          = 0x00000020    # Validity bit for field TGNSSPosition::vSpeed. 
    GNSS_POSITION_HEADING_VALID         = 0x00000040    # Validity bit for field TGNSSPosition::heading. 
    GNSS_POSITION_PDOP_VALID            = 0x00000080    # Validity bit for field TGNSSPosition::pdop. 
    GNSS_POSITION_HDOP_VALID            = 0x00000100    # Validity bit for field TGNSSPosition::hdop. 
    GNSS_POSITION_VDOP_VALID            = 0x00000200    # Validity bit for field TGNSSPosition::vdop. 
    GNSS_POSITION_USAT_VALID            = 0x00000400    # Validity bit for field TGNSSPosition::usedSatellites. 
    GNSS_POSITION_TSAT_VALID            = 0x00000800    # Validity bit for field TGNSSPosition::trackedSatellites. 
    GNSS_POSITION_VSAT_VALID            = 0x00001000    # Validity bit for field TGNSSPosition::visibleSatellites. 
    GNSS_POSITION_SHPOS_VALID           = 0x00002000    # Validity bit for field TGNSSPosition::sigmaHPosition. 
    GNSS_POSITION_SALT_VALID            = 0x00004000    # Validity bit for field TGNSSPosition::sigmaAltitude. 
    GNSS_POSITION_SHSPEED_VALID         = 0x00008000    # Validity bit for field TGNSSPosition::sigmaHSpeed. 
    GNSS_POSITION_SVSPEED_VALID         = 0x00010000    # Validity bit for field TGNSSPosition::sigmaVSpeed. 
    GNSS_POSITION_SHEADING_VALID        = 0x00020000    # Validity bit for field TGNSSPosition::sigmaHeading. 
    GNSS_POSITION_STAT_VALID            = 0x00040000    # Validity bit for field TGNSSPosition::fixStatus. 
    GNSS_POSITION_TYPE_VALID            = 0x00080000    # Validity bit for field TGNSSPosition::fixTypeBits.     
    GNSS_POSITION_ASYS_VALID            = 0x00100000    # Validity bit for field TGNSSPosition::activated_systems. 
    GNSS_POSITION_USYS_VALID            = 0x00200000    # Validity bit for field TGNSSPosition::used_systems.     
    
    
    def __init__(self):
        self.valid = False
        self.timestamp = None
        self.latitude = None
        self.longitude = None
        self.altitudeMSL = None
        self.altitudeEll = None
        self.hSpeed = None
        self.vSpeed = None
        self.heading = None
        self.pdop = None
        self.hdop = None
        self.vdop = None
        self.usedSatellites = None
        self.trackedSatellites = None
        self.visibleSatellites = None
        self.sigmaHPosition = None
        self.sigmaAltitude = None
        self.sigmaHSpeed = None
        self.sigmaVSpeed = None
        self.sigmaHeading = None
        self.fixStatus = None
        self.fixTypeBits = None
        self.activated_systems = None
        self.used_systems = None

    def from_logstring(self, logstring):
        self.__init__()
        logstring = logstring.rstrip() #remove \r\n at end of string
        fields = logstring.split(",")
        if (len(fields) == 27) and (fields[2] == "$GVGNSPOS"):
            #print(logstring)
            self.timestamp = int(fields[3])
            self.validity_bits = int(fields[26],16)
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_LATITUDE_VALID:
                self.latitude = float(fields[4])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_LONGITUDE_VALID:
                self.longitude = float(fields[5])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_ALTITUDEMSL_VALID:
                self.altitudeMSL = float(fields[6])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_ALTITUDEELL_VALID:
                self.altitudeEll = float(fields[7])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_HSPEED_VALID:
                self.hSpeed = float(fields[8])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_VSPEED_VALID:
                self.vSpeed = float(fields[9])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_HEADING_VALID:
                self.heading = float(fields[10])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_PDOP_VALID:
                self.pdop = float(fields[11])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_HDOP_VALID:
                self.hdop = float(fields[12])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_VDOP_VALID:
                self.vdop = float(fields[13])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_USAT_VALID:
                self.usedSatellites = int(fields[14])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_TSAT_VALID:
                self.trackedSatellites = int(fields[15])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_VSAT_VALID:
                self.visibleSatellites = int(fields[16])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_SHPOS_VALID:
                self.sigmaHPosition = float(fields[17])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_SALT_VALID:
                self.sigmaAltitude = float(fields[18])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_SHSPEED_VALID:
                self.sigmaHSpeed = float(fields[19])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_SVSPEED_VALID:
                self.sigmaVSpeed = float(fields[20])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_SHEADING_VALID:
                self.sigmaHeading = float(fields[21])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_STAT_VALID:
                self.fixStatus = int(fields[22])
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_TYPE_VALID:
                self.fixTypeBits = int(fields[23],16)
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_ASYS_VALID:
                self.activated_systems = int(fields[24],16)
            if self.validity_bits & GVGNSPOS.GNSS_POSITION_USYS_VALID:
                self.used_systems = int(fields[25],16)
            self.valid = True
        return self.valid

#FORMAT timestamp,countdown,$GVSNSACC,timestamp,x_acceleration,y_acceleration,z_acceleration,temperature,validityBits
class GVSNSACC:
    """Encapsulate a GVSNSACC sentence.
    
    The attributes correspond to the fields in the corresponding
    C data structure TAccelerationData from header file acceleration.h
    Invalid data are set to None
    """

    ACCELERATION_X_VALID                = 0x00000001    #Validity bit for field TAccelerationData::x.
    ACCELERATION_Y_VALID                = 0x00000002    #Validity bit for field TAccelerationData::y.
    ACCELERATION_Z_VALID                = 0x00000004    #Validity bit for field TAccelerationData::z.
    ACCELERATION_TEMPERATURE_VALID      = 0x00000008    #Validity bit for field TAccelerationData::temperature.

    def __init__(self):
        self.valid = False
        self.timestamp = None
        self.x = None
        self.y = None
        self.z = None
        self.temperature = None
        
    def from_logstring(self, logstring):
        self.__init__()
        logstring = logstring.rstrip() #remove \r\n at end of string
        fields = logstring.split(",")
        if (len(fields) == 9) and (fields[2] == "$GVSNSACC"):
            #print(logstring)
            self.timestamp = int(fields[3])
            self.validity_bits = int(fields[8],16)
            if self.validity_bits & GVSNSACC.ACCELERATION_X_VALID:
                self.x = float(fields[4])
            if self.validity_bits & GVSNSACC.ACCELERATION_Y_VALID:
                self.y = float(fields[5])
            if self.validity_bits & GVSNSACC.ACCELERATION_Z_VALID:
                self.z = float(fields[6])
            if self.validity_bits & GVSNSACC.ACCELERATION_TEMPERATURE_VALID:
                self.temperature = float(fields[7])
            self.valid = True
        return self.valid

#FORMAT timestamp,countdown,$GVSNSGYR,timestamp,yawRate,pitchRate,rollRate,temperature,validityBits
class GVSNSGYR:
    """Encapsulate a GVSNSGYR sentence.
    
    The attributes correspond to the fields in the corresponding
    C data structure TGyroscopeData from header file gyroscope.h
    Invalid data are set to None
    """

    GYROSCOPE_YAWRATE_VALID             = 0x00000001    #Validity bit for field TGyroscopeData::yawRate.
    GYROSCOPE_PITCHRATE_VALID           = 0x00000002    #Validity bit for field TGyroscopeData::pitchRate.
    GYROSCOPE_ROLLRATE_VALID            = 0x00000004    #Validity bit for field TGyroscopeData::rollRate.
    GYROSCOPE_TEMPERATURE_VALID         = 0x00000008    #Validity bit for field TGyroscopeData::temperature.
    
    def __init__(self):
        self.valid = False
        self.timestamp = None
        self.yawRate = None
        self.pitchRate = None
        self.rollRate = None
        self.temperature = None
        
    def from_logstring(self, logstring):
        self.__init__()
        logstring = logstring.rstrip() #remove \r\n at end of string
        fields = logstring.split(",")
        if (len(fields) == 9) and (fields[2] == "$GVSNSGYR"):
            #print(logstring)
            self.timestamp = int(fields[3])
            self.validity_bits = int(fields[8],16)
            if self.validity_bits & GVSNSGYR.GYROSCOPE_YAWRATE_VALID:
                self.yawRate = float(fields[4])
            if self.validity_bits & GVSNSGYR.GYROSCOPE_PITCHRATE_VALID:
                self.pitchRate = float(fields[5])
            if self.validity_bits & GVSNSGYR.GYROSCOPE_ROLLRATE_VALID:
                self.rollRate = float(fields[6])
            if self.validity_bits & GVSNSGYR.GYROSCOPE_TEMPERATURE_VALID:
                self.temperature = float(fields[7])
            self.valid = True
        return self.valid
		
#FORMAT timestamp,countdown,$EXSNSBAR,timestamp,pressure,temperature,validityBits
class EXSNSBAR:
    """Encapsulate a EXSNSBAR sentence.
    
    The attributes correspond to the fields in the corresponding
    C data structure TBarometerData from header file x-barometer.h
    Invalid data are set to None
    """

    BAROMETER_PRESSURE_VALID            = 0x00000001    #Validity bit for field TBarometerData::pressure.
    BAROMETER_TEMPERATURE_VALID         = 0x00000002    #Validity bit for field TBarometerData::temperature.
    
    def __init__(self):
        self.valid = False
        self.timestamp = None
        self.pressure = None
        self.temperature = None
        
    def from_logstring(self, logstring):
        self.__init__()
        logstring = logstring.rstrip() #remove \r\n at end of string
        fields = logstring.split(",")
        if (len(fields) == 7) and (fields[2] == "$EXSNSBAR"):
            #print(logstring)
            self.timestamp = int(fields[3])
            self.validity_bits = int(fields[6],16)
            if self.validity_bits & EXSNSBAR.BAROMETER_PRESSURE_VALID:
                self.pressure = float(fields[4])
            if self.validity_bits & EXSNSBAR.BAROMETER_TEMPERATURE_VALID:
                self.temperature = float(fields[5])
            self.valid = True
        return self.valid		
		
GPXHEADER = """<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="g4l2gpx"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:g4l="huirad"
  xmlns="http://www.topografix.com/GPX/1/1"
  xmlns:gpxx="http://www.garmin.com/xmlschemas/GpxExtensions/v3"
  xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
"""
TRKHEADER = """<trk>
<name>G4L</name>
<trkseg>
"""
TRKFOOTER = """</trkseg>
</trk>
"""
GPXFOOTER = """</gpx>
"""

filename = sys.argv[1]

print(GPXHEADER)
print(TRKHEADER)

with open (filename, "r", encoding='latin_1') as f:  #"r" is default, so it could be left away
    tim = GVGNSTIM()
    pos = GVGNSPOS()
    acc = GVSNSACC()
    gyr = GVSNSGYR()
    bar = EXSNSBAR()
  
    for line in f:
        line = line.rstrip() #remove \r\n at end of string
        #print(line)
        if line.startswith("#"):
            #print(line)
            pass
        elif "$GVSNSACC" in line:
            if acc.from_logstring(line):
                pass
        elif "$GVSNSGYR" in line:
            if gyr.from_logstring(line):
                pass
        elif "$EXSNSBAR" in line:
            if bar.from_logstring(line):
                pass                
        elif "$GVGNSTIM" in line:
            if tim.from_logstring(line):
                pass
        elif "$GVGNSPOS" in line:
            #print(line)
            if pos.from_logstring(line):
                #print(line)
                if pos.fixStatus == 3 and pos.latitude and pos.longitude:
                    print('<trkpt lat="{0}" lon="{1}">'.format(pos.latitude, pos.longitude))
                    if pos.altitudeMSL:
                        print('<ele>{0}</ele>'.format(pos.altitudeMSL))
                    if tim.valid and tim.timestamp == pos.timestamp:
                        #print(tim.year, tim.month, tim.day, tim.hour, tim.minute, tim.second, tim.valid)
                        print('<time>{0:04d}-{1:02d}-{2:02d}T{3:02d}:{4:02d}:{5:02d}Z</time>'.format(
                            #tim.year, tim.month+1, tim.day, 44, 55, 66))
                            tim.year, tim.month+1, tim.day, tim.hour, tim.minute, tim.second))
                    print('<extensions>')
                    if pos.sigmaHPosition:
                        print('<g4p:pos_acc>{0}</g4p:pos_acc>'.format(pos.sigmaHPosition))
                    if acc.x:
                        print('<g4a:x>{0}</g4a:x>'.format(acc.x))
                    if acc.y:
                        print('<g4a:y>{0}</g4a:y>'.format(acc.y))
                    if acc.z:
                        print('<g4a:z>{0}</g4a:z>'.format(acc.z))
                    if acc.temperature:
                        print('<g4a:t>{0}</g4a:t>'.format(acc.temperature))
                    if gyr.yawRate:
                        print('<g4g:Y>{0}</g4g:Y>'.format(gyr.yawRate))
                    if gyr.pitchRate:
                        print('<g4g:P>{0}</g4g:P>'.format(gyr.pitchRate))
                    if gyr.rollRate:
                        print('<g4g:R>{0}</g4g:R>'.format(gyr.rollRate))
                    if bar.pressure:
                        print('<g4b:P>{0}</g4b:P>'.format(bar.pressure))                        
                    print('</extensions>')
                    print('</trkpt>')
        

print(TRKFOOTER)
print(GPXFOOTER)
