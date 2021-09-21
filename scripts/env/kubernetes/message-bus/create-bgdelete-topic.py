#!/usr/bin/python3
from cortx.utils.message_bus import MessageBusAdmin
admin = MessageBusAdmin(admin_id = 'register')
admin.register_message_type(message_types=['bgdelete'], partitions = 1)
