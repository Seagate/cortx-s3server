""" This class generates IEM for background processes
    eventcode part  005 represents s3background component
    and 0001 repesents connection failures """

import syslog


class IEMutil(object):
    severity = ""
    eventCode = ""
    eventstring = ""

    # List of eventcodes
    S3_CONN_FAILURE = "0050010001"
    RABBIT_MQ_CONN_FAILURE = "0050020001"

    # List of eventstrings
    S3_CONN_FAILURE_STR = "failed to connect to s3"
    RABBIT_MQ_CONN_FAILURE_STR = "failed to connect to RabbitMq"

    def __init__(self, loglevel, eventcode, eventstring):
        self.eventCode = eventcode
        self.loglevel = loglevel
        self.eventString = eventstring
        if(loglevel == "INFO"):
            self.severity = "I"
        if(loglevel == "WARN"):
            self.severity = "W"
        if(loglevel == "ERROR"):
            self.severity = "E"

        self.log_iem()

    def log_iem(self):
        IEM = "IEC:%sS%s:%s" % (self.severity, self.eventCode, self.eventString)
        syslog.syslog(IEM)
