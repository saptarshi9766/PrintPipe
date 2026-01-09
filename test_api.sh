#!/bin/bash

echo "=== PrintPipe Virtual Printer Demo ==="
echo

# 1. Create a print job
echo "1. Creating print job..."
JOB_RESPONSE=$(wget -qO- --post-data='{"name": "invoice-2024", "payload": "INVOICE #12345\nDate: Jan 7, 2026\nItem: PrintPipe License\nPrice: $599.99"}' --header='Content-Type: application/json' http://localhost:8080/api/jobs)
echo "$JOB_RESPONSE"
JOB_ID=$(echo "$JOB_RESPONSE" | grep -oP '"job_id":\s*"\K[^"]+')
echo

# 2. Submit for processing
echo "2. Submitting job $JOB_ID for processing..."
wget -qO- --method=POST http://localhost:8080/api/jobs/$JOB_ID/submit
echo
echo

# 3. Wait a moment
echo "3. Waiting for job to process..."
sleep 1
echo

# 4. Check status
echo "4. Checking job status..."
wget -qO- http://localhost:8080/api/jobs/$JOB_ID
echo
echo

# 5. Download output
echo "5. Downloading generated output file..."
wget -qO- http://localhost:8080/api/jobs/$JOB_ID/output
echo
echo

# 6. List all jobs
echo "6. Listing all jobs..."
wget -qO- http://localhost:8080/api/jobs
echo
echo

# 7. View events
echo "7. Viewing event history..."
wget -qO- http://localhost:8080/api/events
echo

echo
echo "=== Demo Complete ==="
echo "Check the 'out/' directory for generated files!"
