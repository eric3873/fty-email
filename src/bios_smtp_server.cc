/*  =========================================================================
    bios_smtp_server - Smtp actor

    Copyright (C) 2014 - 2015 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

/*
@header
    bios_smtp_server - Smtp actor
@discuss
@end
*/

#include "agent_smtp_classes.h"

#include <iterator>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <malamute.h>
#include <bios_proto.h>
#include <math.h>
#include <functional>
#include "email.h"
#include <cxxtools/split.h>

struct AlertDescription_ {
    std::string _description;
    std::string _state;
    std::string _severity;
    std::set<std::string> _actions;
    int64_t _timestamp;
    int64_t _lastNotification;
    int64_t _lastUpdate;

    AlertDescription_(
        const std::string &description,
        const std::string &state,
        const std::string &severity,
        const std::set<std::string> &actions,
        int64_t timestamp) :
            _description (description),
            _state (state),
            _severity (severity),
            _actions (actions),
            _timestamp (timestamp),
            _lastNotification (0),
            _lastUpdate (timestamp)
    {
    };
};
typedef struct AlertDescription_ AlertDescription;

struct ElementDetails_ {
    char _priority;
    std::string _name;
    // TODO be ready for the list of alerts
    std::string _contactName;
    std::string _contactEmail;
};

typedef struct ElementDetails_ ElementDetails;


/*
 * \brief Class that represents an email configutration and
 * is sesponsible for sending the mail
 *
 * TECHNICAL NOTE:
 * All necessary configure information should be immediately
 * propagated to the Smtp class
 */
class EmailConfiguration {
public:


    /*
     * \brief Create an empty configuration
     *
     * State is "unconfigured"
     */
    explicit EmailConfiguration() :
        _smtp{},
        _configured{false}

    {
    }


    /*
     * \brief if the confuguration is already usable
     *
     * \return true  - if it is possible to send email
     *         false - if at least one parameter is missing
     */
    bool isConfigured(void) const {
        return _configured;
    }


    /* \brief Set the host
     *
     * \param host - a host name
     */
    void host (const std::string &host) {
        // TODO set the indicator, that it was configured
        _smtp.host (host);
    }


    /* \brief Set the sender
     *
     * \param from - a sender's email
     */
    void from (const std::string &from) {
        // TODO set the indicator, that it was configured
        _smtp.host (from);
    }


    //TODO: make me private
    Smtp _smtp;


    /*
     * \brief Generate an email body supposed to be send
     *
     * \param alert - an information about the alert
     * \param asset - an information about the asset
     * \param ruleName - a rule name
     *
     * \return string that contains the the body of the email
     */
    static std::string
        generateBody (const AlertDescription &alert,
            const ElementDetails &asset,
            const std::string &ruleName);


    /*
     * \brief Generate an email subject supposed to be send
     *
     * \param alert - an information about the alert
     * \param asset - an information about the asset
     * \param ruleName - a rule name
     *
     * \return string that contains the the subject of the email
     */
    static std::string
        generateSubject (const AlertDescription &alert,
            const ElementDetails &asset,
            const std::string &ruleName);

private:

    /*
     * \brief Generate an email body supposed to be send
     * for active alert
     *
     * \param alert - an information about the alert
     * \param asset - an information about the asset
     * \param ruleName - a rule name
     *
     * \return string that contains the the body of the email
     */
    static std::string
        generateEmailBodyActive (const AlertDescription &alert,
            const ElementDetails &asset,
            const std::string &ruleName);


    /*
     * \brief Generate an email body supposed to be send
     * for resolved alert
     *
     * \param alert - an information about the alert
     * \param asset - an information about the asset
     * \param ruleName - a rule name
     *
     * \return string that contains the the body of the email
     */
    static std::string
        generateEmailBodyResolved (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName);


    /*
     * \brief Generate an email subject supposed to be send
     * for active alert
     *
     * \param alert - an information about the alert
     * \param asset - an information about the asset
     * \param ruleName - a rule name
     *
     * \return string that contains the the subject of the email
     */
    static  std::string
        generateEmailSubjectActive (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName);


    /*
     * \brief Generate an email subject supposed to be send
     * for resolved alert
     *
     * \param alert - an information about the alert
     * \param asset - an information about the asset
     * \param ruleName - a rule name
     *
     * \return string that contains the the subject of the email
     */
    static std::string
        generateEmailSubjectResolved (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName);

    static const std::string _emailBodyActiveAlertTemplate;
    static const std::string _emailSubjectActiveAlertTemplate;
    static const std::string _emailBodyResolvedAlertTemplate;
    static const std::string _emailSubjectResolvedAlertTemplate;

    // flag, that shows if everythinmg is configured
    bool _configured;
};


static std::string
    replaceTokens (const std::string &text,
        const std::string &pattern,
        const std::string &replacement)
{
    std::string result = text;
    size_t pos = 0;
    while( ( pos = result.find(pattern, pos) ) != std::string::npos){
        result.replace(pos, pattern.length(), replacement);
        pos += replacement.length();
    }
    return result;
}


std::string EmailConfiguration::
    generateEmailBodyResolved (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName)
{
    std::string result = _emailBodyResolvedAlertTemplate;
    replaceTokens (result, "${rulename}", ruleName);
    replaceTokens (result, "${assetname}", asset._name);
    replaceTokens (result, "${description}", alert._description);
    return result;
}


std::string EmailConfiguration::
    generateEmailBodyActive (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName)
{
    std::string result = _emailBodyActiveAlertTemplate;
    replaceTokens (result, "${rulename}", ruleName);
    replaceTokens (result, "${assetname}", asset._name);
    replaceTokens (result, "${description}", alert._description);
    replaceTokens (result, "${priority}", std::to_string(asset._priority));
    replaceTokens (result, "${severity}", alert._severity);
    replaceTokens (result, "${state}", alert._state);
    return result;
}


std::string EmailConfiguration::
    generateEmailSubjectResolved (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName)
{
    std::string result = _emailSubjectResolvedAlertTemplate;
    replaceTokens (result, "${rulename}", ruleName);
    replaceTokens (result, "${assetname}", asset._name);
    return result;
}


std::string EmailConfiguration::
    generateEmailSubjectActive (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName)
{
    std::string result = _emailSubjectActiveAlertTemplate;
    replaceTokens (result, "${rulename}", ruleName);
    replaceTokens (result, "${assetname}", asset._name);
    replaceTokens (result, "${description}", alert._description);
    replaceTokens (result, "${priority}", std::to_string(asset._priority));
    replaceTokens (result, "${severity}", alert._severity);
    replaceTokens (result, "${state}", alert._state);
    return result;
}


std::string EmailConfiguration::
    generateBody (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName)
{
    if ( alert._state == "RESOLVED" ) {
        return generateEmailBodyResolved (alert, asset, ruleName);
    }
    else {
        return generateEmailBodyActive (alert, asset, ruleName);
    }
}


std::string EmailConfiguration::
    generateSubject (const AlertDescription &alert,
        const ElementDetails &asset,
        const std::string &ruleName)
{
    if ( alert._state == "RESOLVED" ) {
        return generateEmailSubjectResolved (alert, asset, ruleName);
    }
    else {
        return generateEmailSubjectActive (alert, asset, ruleName);
    }
}


const std::string EmailConfiguration::
    _emailBodyAlertActive = "In the system an alert was detected. \n"
        " Source rule: ${rulename}\n"
        " Asset: ${assetname}\n"
        " Alert priority: P${priority}\n"
        " Alert severity: ${severity}\n"
        " Alert description: ${description}\n"
        " Alert state: ${state}";


const std::string EmailConfiguration::
    _emailSubjectActiveAlert = "${severity} alert on ${assetname} from the rule ${rulename} is active!";


const std::string EmailConfiguration::
    _emailBodyAlertResolved = "In the system an alert was resolved. \n Source rule: ${rulename}\n Asset: ${assetname}\n Alert description: ${description}";


const std::string EmailConfiguration::
    _emailSubjectAlertResolved = "Alert on ${assetname} from the rule ${rulename} was resolved";


class ElementList {
public:

    ElementList() {};

    size_t count (const std::string &assetName) const {
        return _assets.count(assetName);
    };

    bool empty () const {
        return _assets.empty();
    };

    // throws
    const ElementDetails& getElementDetails
        (const std::string &assetName) const
    {
        return _assets.at(assetName);
    };


    void setElementDetails (const std::string &assetName,
        const ElementDetails &elementDetails)
    {
        auto it = _assets.find(assetName);
        if ( it == _assets.cend() ) {
            _assets.emplace (assetName, elementDetails);
        }
        else {
            it->second = elementDetails;
        }
    };
private:
    std::map <std::string, ElementDetails> _assets;
};

class AlertList {
public :
    AlertList(){};

    typedef typename std::map <std::pair<std::string, std::string>, AlertDescription> AlertsConfig;

    AlertsConfig::iterator add(const char *ruleName, const char *asset, const char *description, const char *state, const char *severity, int64_t timestamp, const char *actions);
    void notify(
        const AlertsConfig::iterator it,
        const EmailConfiguration &emailConfiguration,
        const ElementList &elementList);

private:
    // rule_name , asset_name  -> AlertDescription
    AlertsConfig _alerts;
};


AlertList::AlertsConfig::iterator AlertList::
    add(
        const char *ruleName,
        const char *asset,
        const char *description,
        const char *state,
        const char *severity,
        int64_t timestamp,
        const char *actions)
{
    auto alertKey = std::make_pair(ruleName, asset);
    std::set<std::string> actionList;
    std::set<std::string>::iterator actit = actionList.begin();
    cxxtools::split( '/', std::string(actions), std::inserter(actionList, actit) );

    // try to insert a new alert
    auto newAlert = _alerts.emplace(alertKey, AlertDescription (
            description,
            state,
            severity,
            actionList,
            timestamp
        ));
    // newAlert = pair (iterator, bool). true = inserted , false = not inserted
    if ( newAlert.second == true ) {
        // it is the first time we see this alert
        // bacause in the map "alertKey" wasn't found
        return newAlert.first;
    }
    // it is not the first time we see this alert
    auto it = newAlert.first;
    if ( ( it->second._description != description ) ||
            ( it->second._state != state ) ||
            ( it->second._severity != severity ) ||
            ( it->second._actions != actionList ) ||
            ( it->second._timestamp != timestamp ) )
    {
        it->second._description = description;
        it->second._state = state;
        it->second._severity = severity;
        it->second._actions = actionList;
        it->second._timestamp = timestamp;
        // important information changed -> need to notify asap
        it->second._lastUpdate = ::time(NULL);
    }
    else {
        // intentionally left empty
        // nothing changed -> inform according schedule
    }
    return it;
}

static int getNotificationInterval (const std::string &severity, char priority)
{
    // According Aplha document (severity, priority) is mapped onto the time interval [s]
    static const std::map < std::pair<std::string, char>, int> times = {
        { {"CRITICAL", '1'}, 5  * 60},
        { {"CRITICAL", '2'}, 15 * 60},
        { {"CRITICAL", '3'}, 15 * 60},
        { {"CRITICAL", '4'}, 15 * 60},
        { {"CRITICAL", '5'}, 15 * 60},
        { {"WARNING", '1'}, 1 * 60 * 60},
        { {"WARNING", '2'}, 4 * 60 * 60},
        { {"WARNING", '3'}, 4 * 60 * 60},
        { {"WARNING", '4'}, 4 * 60 * 60},
        { {"WARNING", '5'}, 4 * 60 * 60},
        { {"INFO", '1'}, 8 * 60 * 60},
        { {"INFO", '2'}, 24 * 60 * 60},
        { {"INFO", '3'}, 24 * 60 * 60},
        { {"INFO", '4'}, 24 * 60 * 60},
        { {"INFO", '5'}, 24 * 60 * 60}
    };
    auto it = times.find(std::make_pair (severity, priority));
    if ( it == times.cend() ) {
        return 0;
    }
    else {
        return it->second;
    }
}
void AlertList::
    notify(
        const AlertsConfig::iterator it,
        const EmailConfiguration &emailConfiguration,
        const ElementList &elementList)
{
    //pid_t tmptmp = getpid();
    if ( emailConfiguration.isConfigured() ) {
        zsys_error("Mail system is not configured!");
        return;
    }
    // TODO function Need Notify()
    bool needNotify = false;
    int64_t nowTimestamp = ::time(NULL);
    auto &ruleName = it->first.first;
    ElementDetails assetDetailes;
    try {
        assetDetailes = elementList.getElementDetails(it->first.second);
    }
    catch (const std::exception &e ) {
        zsys_error ("CAN'T NOTIFY unknown asset");
        return;
    }
    auto &alertDescription = it->second;
    if  ( alertDescription._lastUpdate > alertDescription._lastNotification ) {
        // Last notification was send BEFORE last important change take place -> need to notify
        needNotify = true;
    }
    else {
        // so, no important changes, but may be we need to notify according the schedule
        if ( alertDescription._state == "RESOLVED" ) {
            // but only for active alerts
            needNotify = false;
        }
        else if ( ( nowTimestamp - alertDescription._lastNotification ) >
                    getNotificationInterval(alertDescription._severity, assetDetailes._priority ) )
             // If  lastNotification + interval < NOW
        {
            // so, we found out that we need to notify according the schedule
            needNotify = true;
        }
    }

    if ( needNotify )
    {
        zsys_debug ("Want to notify");
        try {

            emailConfiguration._smtp.sendmail(
                assetDetailes._contactEmail,
                EmailConfiguration::generateBody (alertDescription, assetDetailes, ruleName),
                EmailConfiguration::generateSubject (alertDescription, assetDetailes, ruleName)
            );
            alertDescription._lastNotification = nowTimestamp;
            zsys_debug ("notification send at %d", nowTimestamp);
        }
        catch (const std::runtime_error& e) {
            zsys_error ("Error: %s", e.what());
            // here we'll handle the error
        }
    }
}


void onAlertReceive (
    bios_proto_t **message,
    AlertList &alertList,
    ElementList &elementList,
    EmailConfiguration &emailConfiguration)
{
    // when some alert message received
    bios_proto_t *messageAlert = *message;
    // check one more time to be sure, that it is an alert message
    if ( bios_proto_id (messageAlert) != BIOS_PROTO_ALERT )
    {
        zsys_error ("message bios_proto is not ALERT!");
        return;
    }
    // decode alert message
    const char *ruleName = bios_proto_rule (messageAlert);
    const char *asset = bios_proto_element_src (messageAlert);
    const char *description = bios_proto_description (messageAlert);
    const char *state = bios_proto_state (messageAlert);
    const char *severity = bios_proto_severity (messageAlert);
    int64_t timestamp = bios_proto_time (messageAlert);
    if( timestamp <= 0 ) {
        timestamp = time(NULL);
    }
    const char *actions = bios_proto_action (messageAlert);

    // 1. Find out information about the element
    if ( elementList.count(asset) == 0 ) {
        zsys_error ("no information is known about the asset, REQ-REP not implemented");
        // try to findout information about the asset
        // TODO
        return;
    }
    // information was found
    ElementDetails assetDetailes = elementList.getElementDetails (asset);

    // 2. add alert to the list of alerts
    auto it = alertList.add (ruleName, asset, description, state, severity, timestamp, actions);
    // notify user about alert
    alertList.notify(it, emailConfiguration, elementList);

    // destroy the message
    bios_proto_destroy (message);
}

void onAssetReceive (
    bios_proto_t **message,
    ElementList &elementList)
{
    // when some asset message received
    assert (message != NULL );
    assert (elementList.empty() || true );

    bios_proto_t *messageAsset = *message;
    // check one more time to be sure, that it is an asset message
    if ( bios_proto_id (messageAsset) != BIOS_PROTO_ASSET )
    {
        zsys_error ("message bios_proto is not ASSET!");
        return;
    }
    // decode alert message
    // the other fields in the messages are not important
    const char *assetName = bios_proto_name (messageAsset);
    zhash_t *ext = bios_proto_ext (messageAsset);
    zhash_t *aux = bios_proto_aux (messageAsset);

    char *priority = (char *) zhash_lookup (aux, "priority");

    // now, we need to get the contact information
    // TODO insert here a code to handle multiple contacts
    char *contact_name = (char *) zhash_lookup (ext, "contact_name");
    char *contact_email = (char *) zhash_lookup (ext, "contact_email");
    ElementDetails newAsset;
    newAsset._priority = priority[0];
    newAsset._name = assetName;
    newAsset._contactName = contact_name;
    newAsset._contactEmail = contact_email;
    elementList.setElementDetails (assetName, newAsset);
}

void
bios_smtp_server (zsock_t *pipe, void* args)
{
    bool verbose = false;

    char *name = (char*) args;

    mlm_client_t *client = mlm_client_new ();

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);

    zsock_signal (pipe, 0);

    AlertList alertList;
    ElementList elementList;
    EmailConfiguration emailConfiguration;
    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, -1);
        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            char *cmd = zmsg_popstr (msg);

            if (streq (cmd, "$TERM")) {
                zstr_free (&cmd);
                zmsg_destroy (&msg);
                goto exit;
            }
            else
            if (streq (cmd, "VERBOSE")) {
                verbose = true;
            }
            else if (streq (cmd, "CONNECT")) {
                char* endpoint = zmsg_popstr (msg);
                int rv = mlm_client_connect (client, endpoint, 1000, name);
                if (rv == -1)
                    zsys_error ("%s: can't connect to malamute endpoint '%s'", name, endpoint);
                zstr_free (&endpoint);
            }
            else if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (client, stream);
                if (rv == -1)
                    zsys_error ("%s: can't set producer on stream '%s'", name, stream);
                zstr_free (&stream);
            }
            else if (streq (cmd, "CONSUMER")) {
                char* stream = zmsg_popstr (msg);
                char* pattern = zmsg_popstr (msg);
                int rv = mlm_client_set_consumer (client, stream, pattern);
                if (rv == -1)
                    zsys_error ("%s: can't set consumer on stream '%s', '%s'", name, stream, pattern);
                zstr_free (&pattern);
                zstr_free (&stream);
            }
            else
            if (streq (cmd, "MSMTP_PATH")) {
                char* path = zmsg_popstr (msg);
                emailConfiguration._smtp.msmtp_path (path);
                zstr_free (&path);
            }
            else
            if (streq (cmd, "CONFIG")) {
                //char* filename = zmsg_popstr (msg);
                // TODO read configuration SMTP
                /*
                */
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
            continue;
        }
        // This agent is a reactive agent, it reacts only on messages
        // and doesn't do anything if there is no messages
        // TODO: probably email also should be send every XXX seconds,
        // even if no alerts were received
        zmsg_t *zmessage = mlm_client_recv (client);
        if ( zmessage == NULL ) {
            continue;
        }
        std::string topic = mlm_client_subject(client);
        if ( verbose ) {
                zsys_debug("Got message '%s'", topic.c_str());
        }
        // There are inputs
        //  - an alert from alert stream
        //  - an asset config message
        //  - an SMTP settings
        if( is_bios_proto (zmessage) ) {
            bios_proto_t *bmessage = bios_proto_decode (&zmessage);
            if( ! bmessage ) {
                zsys_error ("cannot decode bios_proto message, ignore it");
                continue;
            }
            if ( bios_proto_id (bmessage) == BIOS_PROTO_ALERT )  {
                onAlertReceive (&bmessage, alertList, elementList, emailConfiguration);
            }
            else if ( bios_proto_id (bmessage) == BIOS_PROTO_ASSET )  {
                onAssetReceive (&bmessage, elementList);
            }
            else {
                zsys_error ("it is not an alert message, ignore it");
                bios_proto_destroy (&bmessage);
            }
        }
        zmsg_destroy (&zmessage);
    }
exit:
    // TODO save info to persistence before I die
    zpoller_destroy (&poller);
    // TODO need to return to main correctly
    mlm_client_destroy (&client);
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
bios_smtp_server_test (bool verbose)
{
    printf (" * bios_smtp_server: ");

    //  @selftest
    static const char* endpoint = "inproc://bios-ag-server-test";

    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    assert ( server != NULL );
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose) {
        zstr_send (server, "VERBOSE");
    }

    mlm_client_t *producer = mlm_client_new ();
    int rv = mlm_client_connect (producer, endpoint, 1000, "producer");
    assert( rv != -1 );
    rv = mlm_client_set_producer (producer, "ALERTS");
    assert( rv != -1 );

    zactor_t *ag_server = zactor_new (bios_smtp_server, (void*) "agent-smtp");

    if (verbose) {
        zstr_send (ag_server, "VERBOSE");
    }
    zstr_sendx (ag_server, "CONNECT", endpoint, NULL);
    zclock_sleep (500);   //THIS IS A HACK TO SETTLE DOWN THINGS
    zstr_sendx (ag_server, "CONSUMER", "ALERTS",".*", NULL);
    zclock_sleep (500);   //THIS IS A HACK TO SETTLE DOWN THINGS

    zmsg_t *msg = bios_proto_encode_alert (NULL, "NY_RULE", "QWERTY", "ACTIVE","CRITICAL","ASDFKLHJH", 123456, "EMAIL");
    assert (msg);
    std::string atopic = "NY_RULE/CRITICAL@QWERTY";
    mlm_client_send (producer, atopic.c_str(), &msg);

    zclock_sleep (500);   //THIS IS A HACK TO SETTLE DOWN THINGS

    zactor_destroy (&ag_server);
    mlm_client_destroy (&producer);
    zactor_destroy (&server);
    //  @selftest
    //  Simple create/destroy test
    //  @end
    printf ("OK\n");
}