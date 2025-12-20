import logging
import os

def setup_logging(log_level='INFO', log_to_file=False, log_file_path='server.log'):
    level = getattr(logging, log_level.upper(), logging.INFO)
    handlers = [logging.StreamHandler()]
    if log_to_file:
        log_dir = os.path.dirname(log_file_path)
        if log_dir:
            os.makedirs(log_dir, exist_ok=True)
        handlers.append(logging.FileHandler(log_file_path))
    logging.basicConfig(
        level=level,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s',
        handlers=handlers
    )
