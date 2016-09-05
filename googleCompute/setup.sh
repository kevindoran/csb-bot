cloud compute --project "csb-bot" instances create "csb-bot2" --zone "us-west1-a" --machine-type "n1-highcpu-32" --subnet "default" --no-restart-on-failure --maintenance-policy "TERMINATE" --preemptible --scopes default="https://www.googleapis.com/auth/devstorage.read_only","https://www.googleapis.com/auth/logging.write","https://www.googleapis.com/auth/monitoring.write","https://www.googleapis.com/auth/servicecontrol","https://www.googleapis.com/auth/service.management.readonly" --image "/ubuntu-os-cloud/ubuntu-1604-xenial-v20160825" --boot-disk-size "10" --boot-disk-type "pd-standard" --boot-disk-device-name "csb-bot2"




