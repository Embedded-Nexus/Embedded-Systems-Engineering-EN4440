from flask import Blueprint

config_bp = Blueprint('config', __name__)
data_bp = Blueprint('data', __name__)
from app.routes.commands import commands_bp
