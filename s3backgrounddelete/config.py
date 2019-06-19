class Config():

    #endpoint = 'http://s3.seagate.com:80'
    endpoint = "http://127.0.0.1:5000"

class RabbitMQConfig():

    # RabbitMQ overview:  https://www.rabbitmq.com/getstarted.html

    user = "admin"                     # Username used to communicate with RabbitMQ
    password = "password"              # Password corresponding to Username
    host = "127.0.0.1"                 # Host on which RabbitMQ is runnning
    queque = "s3_delete_obj_job_queue" # Queue name used in RabbitMQ
    exchange = ""                      # Exchanges control the routing of messages to queues.
    exchange_type = "direct"           # Exchanges type includes direct, fanout, topic exchange, header exchange
    routings = ""                      # Routing keys are used for specific exchange types
    mode = 2                           # Makes message persistent. Ensures that the task_queue queue won't be lost even if RabbitMQ restarts. 
    durable = "True"                   # Ensure that RabbitMQ will never lose our queue.
    schedule_interval = 900            # Schedule Interval is time period at which object recovery scheduler will be executed

