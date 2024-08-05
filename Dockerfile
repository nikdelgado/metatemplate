# Use the official Python image from the Docker Hub
FROM python:3.10-slim

# Set environment variables
ENV POETRY_VERSION=1.3.2

# Ensure Python output is not buffered (useful for Docker logs)
ENV PYTHONUNBUFFERED=1

# Install Poetry
RUN pip install poetry==$POETRY_VERSION

# Set the working directory
WORKDIR /app

# Copy the pyproject.toml and poetry.lock files
COPY pyproject.toml poetry.lock* /app/

# Install the project dependencies
RUN poetry install --no-root

# Copy the rest of the application code
COPY . /app

# Set the entry point for the container
ENTRYPOINT ["poetry", "run"]
CMD ["python", "-m", "metatemplate", "--help"]