## Steps to configure & run S3backgrounddelete process

----
1. Install & configure RabbbitMQ as per <s3 src>/s3backgrounddelete/rabbitmq_setup.md

2. Configure & run S3 dummyserver as per <s3 src>/st/dummyservers/eos-core-http/readme

3. Add sample data

    python3 tests/create_probable_delete_oid.py
4. Run S3backgrounddelete scheduler

    python3 object_recovery_scheduler.py
5. Run S3backgrounddelete processor

    python3 object_recovery_processor.py

6. Check logs at /var/log/seagate/s3/s3backgrounddelete
----
