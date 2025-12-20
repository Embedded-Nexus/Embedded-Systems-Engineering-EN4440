from app import create_app

def main():
    """Main entry point for the Flask application"""
    app = create_app()
    app.run(host='0.0.0.0', port=5000, debug=True)


if __name__ == "__main__":
    main()
