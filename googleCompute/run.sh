# Instance should be terminated
#gcloud compute instances set-machine-type csb-bot --zone us-west1-b --machine-type n1-standard-1
#gcloud compute instances set-machine-type csb-bot --zone us-west1-b --machine-type n1-highcpu-32

# Start
gcloud compute instances start csb-bot --zone us-west1-b && \

# Copy executable
gcloud compute copy-files ~/projects/csb-bot/bin/Debug/paramSim csb-bot:~/ --zone us-west1-b && \

# Run and pipe to file
gcloud compute ssh csb-bot --zone us-west1-b --command "~/paramSim > out.txt" && \

# Copy output file back
gcloud compute copy-files csb-bot:~/out.txt ~/projects/csb-bot/googleCompute --zone us-west1-b && \

# Close down
gcloud compute instances stop csb-bot --zone us-west1-b
