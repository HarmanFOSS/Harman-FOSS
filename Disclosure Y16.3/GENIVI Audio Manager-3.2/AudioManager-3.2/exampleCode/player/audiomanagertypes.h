/**
 * Copyright (C) 2012, BMW AG
 *
 *
 * \copyright
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * \author Christian Mueller, christian.ei.mueller@bmw.de BMW 2011,2012
 *
 *
 */
#if !defined(EA_F9B4F59D_FED5_44ac_85F2_F9F60549C133__INCLUDED_)
#define EA_F9B4F59D_FED5_44ac_85F2_F9F60549C133__INCLUDED_

#include <stdint.h>
#include "projecttypes.h"
#include <string>
#include <vector>

#define AM_MUTE -3000

namespace am {
    /**
     * a domain ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_domainID_t;

    /**
     * a source ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_sourceID_t;

    /**
     * a sink ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_sinkID_t;

    /**
     * a gateway ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_gatewayID_t;

    /**
     * a crossfader ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_crossfaderID_t;

    /**
     * a connection ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_connectionID_t;

    /**
     * a mainConnection ID
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_mainConnectionID_t;

    /**
     * speed
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_speed_t;

    /**
     * The unit is 0.1 db steps,The smallest value -3000 (=AM_MUTE). The minimum and
     * maximum can be limited by actual project.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef int16_t am_volume_t;

    /**
     * This is the volume presented on the command interface. It is in the duty of the
     * Controller to change the volumes given here into meaningful values on the
     * routing interface.
     * The range of this type is customer specific.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef int16_t am_mainVolume_t;

    /**
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_sourceClass_t;

    /**
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_sinkClass_t;

    /**
     * time in ms!
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef uint16_t am_time_t;

    /**
     * offset time that is introduced in milli seconds.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:13 PM
     */
    typedef int16_t am_timeSync_t;

    /**
     * with the help of this enum, sinks and sources can report their availability
     * state
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_Availability_e
    {
        /**
         * default
         */
        A_UNKNOWN = 0,
        /**
         * The source / sink is available
         */
        A_AVAILABLE = 1,
        /**
         * the source / sink is not available
         */
        A_UNAVAILABLE = 2,
        A_MAX
    };

    /**
     * represents the connection state
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_ConnectionState_e
    {
        CS_UNKNOWN = 0,
        /**
         * This means the connection is just building up
         */
        CS_CONNECTING = 1,
        /**
         * the connection is ready to be used
         */
        CS_CONNECTED = 2,
        /**
         * the connection is in the course to be knocked down
         */
        CS_DISCONNECTING = 3,
        /**
         * only relevant for connectionStatechanged. Is send after the connection was
         * removed
         */
        CS_DISCONNECTED = 4,
        /**
         * this means the connection is still build up but unused at the moment
         */
        CS_SUSPENDED = 5,
        CS_MAX
    };

    /**
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_DomainState_e
    {
        /**
         * default
         */
        DS_UNKNOWN = 0,
        /**
         * the domain is controlled by the daemon
         */
        DS_CONTROLLED = 1,
        /**
         * the domain is independent starting up
         */
        DS_INDEPENDENT_STARTUP = 1,
        /**
         * the domain is independent running down
         */
        DS_INDEPENDENT_RUNDOWN = 2,
        DS_MAX
    };

    /**
     * This enum characterizes the data of the EarlyData_t
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_EarlyDataType_e
    {
        /**
         * default
         */
        ES_UNKNOWN = 0,
        /**
         * the source volume
         */
        ED_SOURCE_VOLUME = 1,
        /**
         * the sink volume
         */
        ED_SINK_VOLUME = 2,
        /**
         * a source property
         */
        ED_SOURCE_PROPERTY = 3,
        /**
         * a sink property
         */
        ED_SINK_PROPERTY = 4,
        ED_MAX
    };

    /**
     * the errors of the audiomanager. All possible errors are in here. This enum is
     * used widely as return parameter.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_Error_e
    {
        /**
         * no error - positive reply
         */
        E_OK = 0,
        /**
         * default
         */
        E_UNKNOWN = 1,
        /**
         * value out of range
         */
        E_OUT_OF_RANGE = 2,
        /**
         * not used
         */
        E_NOT_USED = 3,
        /**
         * a database error occurred
         */
        E_DATABASE_ERROR = 4,
        /**
         * the desired object already exists
         */
        E_ALREADY_EXISTS = 5,
        /**
         * there is no change
         */
        E_NO_CHANGE = 6,
        /**
         * the desired action is not possible
         */
        E_NOT_POSSIBLE = 7,
        /**
         * the desired object is non existent
         */
        E_NON_EXISTENT = 8,
        /**
         * the asynchronous action was aborted
         */
        E_ABORTED = 9,
        /**
         * This error is returned in case a connect is issued with a connectionFormat that
         * cannot be selected for the connection. This could be either due to the
         * capabilities of a source or a sink or gateway compatibilities for example
         */
        E_WRONG_FORMAT = 10,
        E_MAX
    };

    /**
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_MuteState_e
    {
        /**
         * default
         */
        MS_UNKNOWN = 0,
        /**
         * the source / sink is muted
         */
        MS_MUTED = 1,
        /**
         * the source / sink is unmuted
         */
        MS_UNMUTED = 2,
        MS_MAX
    };

    /**
     * The source state reflects the state of the source
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_SourceState_e
    {
        SS_UNKNNOWN = 0,
        /**
         * The source can be activly heared
         */
        SS_ON = 1,
        /**
         * The source cannot be heared
         */
        SS_OFF = 2,
        /**
         * The source is paused. Meaning it cannot be heared but should be prepared to
         * play again soon.
         */
        SS_PAUSED = 3,
        SS_MAX
    };

    /**
     * This enumeration is used to define the type of the action that is correlated to
     * a handle.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_Handle_e
    {
        H_UNKNOWN = 0,
        H_CONNECT = 1,
        H_DISCONNECT = 2,
        H_SETSOURCESTATE = 3,
        H_SETSINKVOLUME = 4,
        H_SETSOURCEVOLUME = 5,
        H_SETSINKSOUNDPROPERTY = 6,
        H_SETSOURCESOUNDPROPERTY = 7,
        H_SETSINKSOUNDPROPERTIES = 8,
        H_SETSOURCESOUNDPROPERTIES = 9,
        H_CROSSFADE = 10,
        H_MAX
    };

    /**
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_InterruptState_e
    {
        /**
         * default
         */
        IS_UNKNOWN = 0,
        /**
         * the interrupt state is off - no interrupt
         */
        IS_OFF = 1,
        /**
         * the interrupt state is interrupted - the interrupt is active
         */
        IS_INTERRUPTED = 2,
        IS_MAX
    };

    /**
     * describes the active sink of a crossfader.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    enum am_HotSink_e
    {
        /**
         * default
         */
        HS_UNKNOWN = 0,
        /**
         * sinkA is active
         */
        HS_SINKA = 1,
        /**
         * sinkB is active
         */
        HS_SINKB = 2,
        /**
         * the crossfader is in the transition state
         */
        HS_INTERMEDIATE = 3,
        HS_MAX
    };

    /**
     * this describes the availability of a sink or a source together with the latest
     * change
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    struct am_Availability_s
    {

    public:
        /**
         * the current availability state
         */
        am_Availability_e availability;
        /**
         * the reason for the last change. This can be used to trigger events that deal
         * with state changes.
         */
        am_AvailabilityReason_e availabilityReason;

    };

    /**
     * describes class properties
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    struct am_ClassProperty_s
    {

    public:
        /**
         * the property as enum
         */
        am_ClassProperty_e classProperty;
        /**
         * the value of the property
         */
        int16_t value;

    };

    /**
     * This struct describes the attribiutes of a crossfader.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    struct am_Crossfader_s
    {

    public:
        /**
         * This is the ID of the crossfader, it is unique in the system. There are 2 ways,
         * ID can be created: either it is assigned during the registration process (in a
         * dynamic context, uniqueness will be ensured by the AudioManager daemon), or it
         * is a fixed (the project has to ensure the uniqueness of the ID).
         */
        am_crossfaderID_t crossfaderID;
        /**
         * The name of the crossfader. Must be unique in the whole system.
         */
        std::string name;
        /**
         * The sinkID of the SinkA. Sinks shall be registered before registering the
         * crossfader.
         */
        am_sinkID_t sinkID_A;
        /**
         * The sinkID of the SinkB. Sinks shall be registered before registering the
         * crossfader.
         */
        am_sinkID_t sinkID_B;
        /**
         * The sourceID of the crossfader source. The source shall be registered before
         * the crossfader.
         */
        am_sourceID_t sourceID;
        /**
         * This enum can have 3 states:
         *
         *    HS_SINKA sinkA is the current hot one, sinkB is not audible
         *    HS_SINKB sinkB is the current hot one, sinkB is not audible
         *    HS_INTERMEDIATE the fader is stuck in between a cross-fading action. This
         * could be due to an abort or an error. Before using the crossfader, it must be
         * set to either HS_SINKA or HS_SINKB.
         */
        am_HotSink_e hotSink;

    };

    /**
     * This struct describes the attributes of a gateway.
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    struct am_Gateway_s
    {

    public:
        /**
         * This is the ID of the gateway, it is unique in the system. There are 2 ways, ID
         * can be created: either it is assigned during the registration process (in a
         * dynamic context, uniqueness will be ensured by the AudioManagerDaemon), or it
         * is a fixed (the project has to ensure the uniqueness of the ID).
         */
        am_gatewayID_t gatewayID;
        /**
         * The name of the gateway. Must be unique in the whole system.
         */
        std::string name;
        /**
         * The sinkID of the gateway sink-end. The sink is a full blown sink with
         * connectionFormats, sinkClassIDs etc... It makes sense to register the sinks of
         * a gateway as non-visible. Care needs to be taken that the connectionsFormats
         * match with the ones in the conversionMatrix. If the sink is located in the
         * controllingDomain, the ID needs to be retrieved by registering the sink before
         * registering the gateway. In case the sink is in a different domain, the ID
         * needs to be retrieved via peeking.
         */
        am_sinkID_t sinkID;
        /**
         * The sourceID of the gateway sink-end. The sink is a full blown source with
         * connectionFormats, sinkClassIDs etc... It makes sense to register the sources
         * of a gateway as non-visible. Care needs to be taken that the connectionsFormats
         * match with the ones in the conversionMatrix. If the source is located in the
         * controllingDomain, the ID needs to be retrieved by registering the source
         * before registering the gateway. In case the source is in a different domain,
         * the ID needs to be retrieved via peeking.
         */
        am_sourceID_t sourceID;
        /**
         * The ID of the sink. If the domain is the same like the controlling domain, the
         * ID is known due to registration. If the domain is different, the ID needs to be
         * retrieved via peeking.
         */
        am_domainID_t domainSinkID;
        /**
         * The ID of the source. If the domain is the same like the controlling domain,
         * the ID is known due to registration. If the domain is different, the ID needs
         * to be retrieved via peeking.
         */
        am_domainID_t domainSourceID;
        /**
         * This is the ID of the domain that registers the gateway.
         */
        am_domainID_t controlDomainID;
        /**
         * This is the list of available formats on the source side of the gateway. It is
         * not defined during the gateway registration but copied from the source
         * registration.
         */
        std::vector<am_ConnectionFormat_e> listSourceFormats;
        /**
         * This is the list of available formats on the sink side of the gateway. It is
         * not defined during the gateway registration but copied from the sink
         * registration.
         */
        std::vector<am_ConnectionFormat_e> listSinkFormats;
        /**
         * This is matrix holding information about the conversion capability of the
         * gateway, it's length is defined by the length(listSinkFormats) x
         * length(listSourceFormats).
         * If a SinkFormat can be converted into a SourceFormat, the vector will hold a 1,
         * if no conversion is possible, a 0.
         * The data is stored row orientated, where the rows are related to the
         * sinksFormats and the columns to the sourceFormats. The first value will hold
         * the conversion information from the first sourceFormat to the first sinkFormat
         * for example and the seventh value the information about the 3rd sinkFormat to
         * the 1st sourceFormat in case we would have 3 sourceFormats.
         *
         * This matrix
         * 110 011 000 111 001
         *
         * reads as this:
         *          Source
         * 	**  1  2  3
         * *********************
         * S	1*  1  1  0
         * i	2*  0  1  1
         * n	3*  0  0  0
         * k	4*  1  1  1
         * 	5*  0  0  1
         */
        std::vector<bool> convertionMatrix;

    };

    /**
     * This represents one "hopp" in a route
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    struct am_RoutingElement_s
    {

    public:
        /**
         * the source ID
         */
        am_sourceID_t sourceID;
        /**
         * the sinkID
         */
        am_sinkID_t sinkID;
        /**
         * the domainID the routeElement is in
         */
        am_domainID_t domainID;
        /**
         * the connectionformat that is used for the route
         */
        am_ConnectionFormat_e connectionFormat;

    };

    /**
     * a list of routing elements that lead from source to sink
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:14 PM
     */
    struct am_Route_s
    {

    public:
        /**
         * the sourceID where the route starts
         */
        am_sourceID_t sourceID;
        /**
         * the sinkID where the route ends
         */
        am_sinkID_t sinkID;
        /**
         * the actual route as list of routing elements
         */
        std::vector<am_RoutingElement_s> route;

    };

    /**
     * struct describing the sound property
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_SoundProperty_s
    {

    public:
        /**
         * the type of the property - a project specific enum
         */
        am_SoundPropertyType_e type;
        /**
         * the actual value of the property
         */
        int16_t value;

    };

    /**
     * struct describing system properties
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_SystemProperty_s
    {

    public:
        /**
         * the type that is set
         */
        am_SystemPropertyType_e type;
        /**
         * the value
         */
        int16_t value;

    };

    /**
     * struct describing sinkclasses
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_SinkClass_s
    {

    public:
        /**
         * the ID of the sinkClass
         */
        am_sinkClass_t sinkClassID;
        /**
         * the name of the sinkClass - must be unique in the system
         */
        std::string name;
        /**
         * the list of the class properties. These are pairs of  a project specific enum
         * describing the type of the value and an integer holding the real value.
         */
        std::vector<am_ClassProperty_s> listClassProperties;

    };

    /**
     * struct describing source classes
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_SourceClass_s
    {

    public:
        /**
         * the source ID
         */
        am_sourceClass_t sourceClassID;
        /**
         * the name of the sourceClass - must be unique in the system
         */
        std::string name;
        /**
         * the list of the class properties. These are pairs of  a project specific enum
         * describing the type of the value and an integer holding the real value.
         */
        std::vector<am_ClassProperty_s> listClassProperties;

    };

    /**
     * this type holds all information of sources relevant to the HMI
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_SourceType_s
    {

    public:
        /**
         * This is the ID of the source, it is unique in the system. There are 2 ways, ID
         * can be created: either it is assigned during the registration process (in a
         * dynamic context, uniqueness will be ensured by the AudioManagerDaemon), or it
         * is a fixed (the project has to ensure the uniqueness of the ID).
         */
        am_sourceID_t sourceID;
        /**
         * The name of the source. Must be unique in the whole system.
         */
        std::string name;
        /**
         * the availability of the source
         */
        am_Availability_s availability;
        /**
         * the sourceClassID, indicates the class the source is in. This information can
         * be used by the Controller to implement different behaviour for different
         * classes.
         */
        am_sourceClass_t sourceClassID;

    };

    /**
     * this type holds all information of sinks relevant to the HMI
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_SinkType_s
    {

    public:
        /**
         * This is the ID of the sink, it is unique in the system. There are 2 ways, ID
         * can be created: either it is assigned during the registration process (in a
         * dynamic context, uniqueness will be ensured by the AudioManagerDaemon), or it
         * is a fixed (the project has to ensure the uniqueness of the ID).
         */
        am_sinkID_t sinkID;
        /**
         * The name of the sink. Must be unique in the whole system.
         */
        std::string name;
        /**
         * This attribute reflects the availability of the sink. There are several reasons
         * why a sink could be not available for the moment: for example the shutdown of a
         * sink because of overtemperature or over- & undervoltage. The availability
         * consists of two pieces of information:
         *
         *    Availablility: the status itself, can be A_AVAILABLE, A_UNAVAILABLE or
         * A_UNKNOWN
         *    AvailabilityReason: this informs about the last reason for a change in
         * availability. The reasons itself are product specific.
         */
        am_Availability_s availability;
        /**
         * This is the representation of the Volume for the commandInterface. It is used
         * by the HMI to set the volume of a sink, the AudioManagerController has to
         * transform this into real source and sink volumes.
         */
        am_mainVolume_t volume;
        am_MuteState_e muteState;
        /**
         * The sinkClassID references to a sinkClass. With the help of classification,
         * rules can be setup to define the system behaviour.
         */
        am_sinkClass_t sinkClassID;

    };

    /**
     * a handle is used for asynchronous operations and is uniquely assigned for each
     * of this operations
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_Handle_s
    {

    public:
        /**
         * the handletype
         */
        am_Handle_e handleType:4;
        /**
         * the handle as value
         */
        uint16_t handle:12;

    };

    /**
     * struct describung mainsound property
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_MainSoundProperty_s
    {

    public:
        /**
         * the type of the property
         */
        am_MainSoundPropertyType_e type;
        /**
         * the actual value
         */
        int16_t value;

    };

    /**
     * this type holds all information of connections relevant to the HMI
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:15 PM
     */
    struct am_MainConnectionType_s
    {

    public:
        /**
         * the ID of the mainconnection
         */
        am_mainConnectionID_t mainConnectionID;
        /**
         * the sourceID where the connection starts
         */
        am_sourceID_t sourceID;
        /**
         * the sinkID where the connection ends
         */
        am_sinkID_t sinkID;
        /**
         * the delay of the mainconnection
         */
        am_timeSync_t delay;
        /**
         * the current connection state
         */
        am_ConnectionState_e connectionState;

    };

    /**
     * struct that holds attribiutes of a mainconnection
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    struct am_MainConnection_s
    {

    public:
        /**
         * the assigned ID
         */
        am_mainConnectionID_t mainConnectionID;
        /**
         * the current connection state
         */
        am_ConnectionState_e connectionState;
        /**
         * the sinkID
         */
        am_sinkID_t sinkID;
        /**
         * the sourceID
         */
        am_sourceID_t sourceID;
        /**
         * the delay of the connection
         */
        am_timeSync_t delay;
        /**
         * the list of sub connection IDs the mainconnection consists of
         */
        std::vector<am_connectionID_t> listConnectionID;

    };

    /**
     * This struct describes the attribiutes of a sink
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    struct am_Sink_s
    {

    public:
        /**
         * This is the ID of the sink, it is unique in the system. There are 2 ways, ID
         * can be created: either it is assigned during the registration process (in a
         * dynamic context, uniqueness will be ensured by the AudioManagerDaemon), or it
         * is a fixed (the project has to ensure the uniqueness of the ID).
         */
        am_sinkID_t sinkID;
        /**
         * The name of the sink. Must be unique in the whole system.
         */
        std::string name;
        /**
         * The domainID is the domain the sink belongs to. A sink can only be in one
         * domain.
         */
        am_domainID_t domainID;
        /**
         * The sinkClassID references to a sinkClass. With the help of classification,
         * rules can be setup to define the system behaviour.
         */
        am_sinkClass_t sinkClassID;
        /**
         * This is the volume of the sink. It is set by the AudioManagerController.
         */
        am_volume_t volume;
        /**
         * This Boolean flag indicates whether a sink is visible to the commandInterface
         * or not. If the User must have the possibility to choose the source in the HMI,
         * it must be visible. But there are also good reasons for invisible sinks, for
         * example if the sink is part of a crossfader or gateway. HMI relevant changes in
         * visible sinks will be automatically reported by the daemon to the
         * commandInterface.
         */
        bool visible;
        /**
         * This attribute reflects the availability of the sink. There are several reasons
         * why a sink could be not available for the moment: for example the shutdown of a
         * sink because of overtemperature or over- & undervoltage. The availability
         * consists of two pieces of information:
         *
         *    Availablility: the status itself, can be A_AVAILABLE, A_UNAVAILABLE or
         * A_UNKNOWN
         *    AvailabilityReason: this informs about the last reason for a change in
         * availability. The reasons itself are product specific.
         */
        am_Availability_s available;
        /**
         * This attribute reflects the muteState of the sink. The information is not the
         * "real" state of the sink, but the HMI representation for he commandInterface
         * controlled by the AudioManagerController.
         */
        am_MuteState_e muteState;
        /**
         * This is the representation of the Volume for the commandInterface. It is used
         * by the HMI to set the volume of a sink, the AudioManagerController has to
         * transform this into real source and sink volumes.
         */
        am_mainVolume_t mainVolume;
        /**
         * This is the list of soundProperties, that the sink is capable of. The
         * soundProperties itself are project specific. For sinks, a possible
         * soundProperty could be for example settings.
         */
        std::vector<am_SoundProperty_s> listSoundProperties;
        /**
         * This list holds information about the formats that the Source is capable of
         * supporting when delivering audio.
         */
        std::vector<am_ConnectionFormat_e> listConnectionFormats;
        /**
         * This is the list of the available mainSoundProperties. The principle is the
         * same than with soundProperties, but they are only visible to the
         * CommandInterface.
         */
        std::vector<am_MainSoundProperty_s> listMainSoundProperties;

    };

    /**
     * This struct describes the attribiutes of a source
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    struct am_Source_s
    {

    public:
        /**
         * This is the ID of the source, it is unique in the system. There are 2 ways, ID
         * can be created: either it is assigned during the registration process (in a
         * dynamic context, uniqueness will be ensured by the AudioManagerDaemon), or it
         * is a fixed (the project has to ensure the uniqueness of the ID).
         */
        am_sourceID_t sourceID;
        /**
         * The domainID is the domain the source belongs to. A source can only be in one
         * domain.
         */
        am_domainID_t domainID;
        /**
         * The name of the source. Must be unique in the whole system.
         */
        std::string name;
        /**
         * the sourceClassID, indicates the class the source is in. This information can
         * be used by the Controller to implement different behaviour for different
         * classes.
         */
        am_sourceClass_t sourceClassID;
        /**
         * The source state is an indication towards the source if it is actively heard or
         * not. The source can use this information to implement features like automatic
         * spin down of CD's in case the CD is not the active source or AF following of a
         * tuner that is not actively heard. The source state is set by the
         * AudioManagerController.There are 3 possible states:
         *
         *    SS_ON: the source is active
         *    SS_OFF: the source is off
         *    SS_PAUSED: the source is paused and not active.
         */
        am_SourceState_e sourceState;
        /**
         * This is the volume of the source. It is set by the AudioManagerController. It
         * is used to adopt different audiolevels in a system and mixing of sources (e.g.
         * navigation hints & music).
         */
        am_volume_t volume;
        /**
         * This Boolean flag indicates whether a source is visible to the commandInterface
         * or not. If the User must have the possibility to choose the source in the HMI,
         * it must be visible. But there are also good reasons for invisible sources, for
         * example if the source is part of a crossfader or gateway. HMI relevant changes
         * in visible sources will be automatically reported by the daemon to the
         * commandInterface.
         */
        bool visible;
        /**
         * This attribute reflects the availability of the source. There are several
         * reasons why a source could be not available for the moment. For example a CD
         * player which has no CD entered in the slot can be unavailable, or a USB player
         * with no or unreadable stick attached. Other scenarios involve the shutdown of a
         * source because of overtemperature or over- & undervoltage. The availability
         * consists of two informations:
         *
         *    Availablility: the status itself, can be A_AVAILABLE, A_UNAVAILABLE or
         * A_UNKNOWN
         *    AvailabilityReason: this informs about the last reason for a change in
         * availability. The reasons itself are product specific.
         */
        am_Availability_s available;
        /**
         * Some special sources can have special behaviors, the are so called "Low Level
         * Interrupts". Here the current status is documented. The information can be used
         * by the AudioManagerController to react to the changes by for example lowering
         * the volume of the mainSources. The two states are
         *
         *    IS_OFF: the interrupt is not active at the moment
         *    IS_INTERRUPTED: the interrupt is playing at the moment.
         */
        am_InterruptState_e interruptState;
        /**
         * This is the list of soundProperties, that the source is capable of. The
         * soundProperties itself are project specific. For sources, a possible
         * soundProperty could be navigation volume offset, for example.
         */
        std::vector<am_SoundProperty_s> listSoundProperties;
        /**
         * This list holds information about the formats that the Source is capable of
         * supporting when delivering audio.
         */
        std::vector<am_ConnectionFormat_e> listConnectionFormats;
        /**
         * This is the list of the available mainSoundProperties. The principle is the
         * same than with soundProperties, but they are only visible to the
         * CommandInterface.
         */
        std::vector<am_MainSoundProperty_s> listMainSoundProperties;

    };

    /**
     * This struct describes the attribiutes of a domain
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    struct am_Domain_s
    {

    public:
        /**
         * the domain ID
         */
        am_domainID_t domainID;
        /**
         * the name of the domain
         */
        std::string name;
        /**
         * the busname. This is equal to a plugin name and is used to dispatch messages to
         * the elements of a plugin
         */
        std::string busname;
        /**
         * the name of the node
         */
        std::string nodename;
        /**
         * indicated if the domain is independent at startup or not
         */
        bool early;
        /**
         * indicates if the domain registration is complete or not
         */
        bool complete;
        /**
         * the current domain state
         */
        am_DomainState_e state;

    };

    /**
     * a connection
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    struct am_Connection_s
    {

    public:
        /**
         * the assigned ID
         */
        am_connectionID_t connectionID;
        /**
         * the source the audio flows from
         */
        am_sourceID_t sourceID;
        /**
         * the sink the audio flows to
         */
        am_sinkID_t sinkID;
        /**
         * the delay of the conneciton
         */
        am_timeSync_t delay;
        /**
         * the used connectionformat
         */
        am_ConnectionFormat_e connectionFormat;

    };

    /**
     * data type depends of am_EarlyDataType_e:
     * volume_t in case of ED_SOURCE_VOLUME, ED_SINK_VOLUME
     * soundProperty_t in case of ED_SOURCE_PROPERTY, ED_SINK_PROPERTY
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    union am_EarlyData_u
    {

    public:
        am_volume_t volume;
        am_SoundProperty_s soundProperty;

    };

    /**
     * data type depends of am_EarlyDataType_e:
     * sourceID in case of ED_SOURCE_VOLUME, ED_SOURCE_PROPERTY
     * sinkID in case of ED_SINK_VOLUME, ED_SINK_PROPERTY
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:16 PM
     */
    union am_DataType_u
    {

    public:
        am_sinkID_t sink;
        am_sourceID_t source;

    };

    /**
     * @author Christian Mueller
     * @created 07-Mar-2012 6:06:17 PM
     */
    struct am_EarlyData_s
    {

    public:
        am_EarlyDataType_e type;
        am_DataType_u sinksource;
        am_EarlyData_u data;

    };
}
#endif // !defined(EA_F9B4F59D_FED5_44ac_85F2_F9F60549C133__INCLUDED_)
