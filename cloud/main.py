from app import create_app
import os

def main():
    """Main entry point for the Flask application"""
    app = create_app()
    
    # Check if SSL certificates exist for HTTPS
    ssl_cert = os.path.join(os.path.dirname(__file__), 'cert.pem')
    ssl_key = os.path.join(os.path.dirname(__file__), 'key.pem')
    
    if os.path.exists(ssl_cert) and os.path.exists(ssl_key):
        # Run with HTTPS
        print("🔒 Starting server with HTTPS...")
        app.run(
            host='0.0.0.0', 
            port=5000, 
            debug=True,
            ssl_context=(ssl_cert, ssl_key)
        )
    else:
        # Run with HTTP only
        print("⚠️  Starting server with HTTP (no SSL certificates found)")
        print("📝 To enable HTTPS, generate certificates:")
        print("   openssl req -x509 -newkey rsa:4096 -nodes -out cert.pem -keyout key.pem -days 365")
        app.run(host='0.0.0.0', port=5000, debug=True)


if __name__ == "__main__":
    main()
