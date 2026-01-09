# PrintPipe HTTP Server

HTTP REST API frontend for the PrintPipe virtual printer driver.

## Features

- **RESTful API** for job submission and management
- **JSON-based** request/response format
- **Event tracking** for all job state transitions
- **Thread-safe** job queue with atomic state management

## Building

```bash
mkdir -p build && cd build
cmake ..
make printpipe_http_server
```

## Running the Server

```bash
# Default port 8080
./build/printpipe_http_server

# Custom port
./build/printpipe_http_server 9000
```

## API Endpoints

### `GET /`
Server information and available endpoints.

```bash
curl http://localhost:8080/
```

### `POST /api/jobs`
Create a new print job.

**Request:**
```bash
curl -X POST http://localhost:8080/api/jobs \
  -H 'Content-Type: application/json' \
  -d '{"name": "my-document"}'
```

**Response:**
```json
{
  "job_id": "job-000000",
  "status": "created"
}
```

### `POST /api/jobs/:id/submit`
Submit a job for processing (queuing, spooling, printing).

**Request:**
```bash
curl -X POST http://localhost:8080/api/jobs/job-000000/submit
```

**Response:**
```json
{
  "job_id": "job-000000",
  "status": "processing"
}
```

### `GET /api/jobs/:id`
Get the status of a specific job.

**Request:**
```bash
curl http://localhost:8080/api/jobs/job-000000
```

**Response:**
```json
{
  "job_id": "job-000000",
  "name": "my-document",
  "state": "completed",
  "has_buffer": true
}
```

### `GET /api/jobs`
List all jobs.

**Request:**
```bash
curl http://localhost:8080/api/jobs
```

**Response:**
```json
[
  {
    "job_id": "job-000000",
    "name": "my-document",
    "state": "completed",
    "has_buffer": true
  },
  {
    "job_id": "job-000001",
    "name": "invoice",
    "state": "printing",
    "has_buffer": true
  }
]
```

### `GET /api/events`
Get all events from the event bus.

**Request:**
```bash
curl http://localhost:8080/api/events
```

**Response:**
```json
[
  {
    "kind": "state_changed",
    "job_name": "my-document",
    "from": "created",
    "to": "queued",
    "reason": ""
  },
  {
    "kind": "state_changed",
    "job_name": "my-document",
    "from": "queued",
    "to": "scheduled",
    "reason": ""
  }
]
```

## Job States

- `created` - Job created but not yet queued
- `queued` - Job in the print queue
- `scheduled` - Job scheduled for processing
- `spooling` - Job being spooled
- `printing` - Job being printed
- `completed` - Job successfully completed
- `canceled` - Job was canceled
- `failed` - Job failed during processing

## Example Workflow

```bash
# 1. Create a job
RESPONSE=$(curl -s -X POST http://localhost:8080/api/jobs \
  -H 'Content-Type: application/json' \
  -d '{"name": "test-document"}')

JOB_ID=$(echo $RESPONSE | jq -r '.job_id')
echo "Created job: $JOB_ID"

# 2. Submit the job for processing
curl -X POST http://localhost:8080/api/jobs/$JOB_ID/submit

# 3. Check job status
curl http://localhost:8080/api/jobs/$JOB_ID

# 4. View all events
curl http://localhost:8080/api/events
```

## Dependencies

- **cpp-httplib** - HTTP server library (header-only)
- **nlohmann/json** - JSON parsing and serialization (header-only)

Both dependencies are automatically fetched via CMake FetchContent.

## Architecture

The HTTP server (`PrintServer` class):
1. Receives job creation requests via REST API
2. Creates `Job` objects with unique IDs
3. Manages job lifecycle through state transitions
4. Uses `TextSpooler` to generate print buffers
5. Publishes events to `EventBus` for monitoring
6. Returns JSON responses for all operations

All job state transitions are atomic and thread-safe.
