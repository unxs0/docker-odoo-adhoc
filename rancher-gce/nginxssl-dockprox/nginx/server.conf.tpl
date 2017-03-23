server {
    listen 80;
    server_name {{cPublicServerName}};

    if ($scheme != "https") {
        return 301 https://$host$request_uri;
    } # managed by Certbot

    # proxy header and settings
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-Host $host;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    add_header X-Content-Type-Options nosniff;
    proxy_redirect off;

    # odoo log files
    access_log /var/log/nginx/access_{{cUpstreamServerName}};
    error_log  /var/log/nginx/error_{{cUpstreamServerName}};

    # increase proxy buffer size
    proxy_buffers 16 64k;
    proxy_buffer_size 128k;

    # force timeouts if the backend dies
    proxy_next_upstream error timeout invalid_header http_500 http_502 http_503;

    # enable data compression
    gzip on;
    gzip_min_length 1100;
    gzip_buffers 4 32k;
    gzip_types text/plain application/x-javascript text/xml text/css;
    gzip_vary on;

    # cache some static data in memory for 60mins.
    # under heavy load this should relieve stress on the OpenERP web interface a bit.
    location ~* /web/static/ {
        proxy_cache_valid 200 60m;
        proxy_buffering    on;
        expires 864000;
        proxy_pass http://{{cUpstreamServerName}};
    }

    location / {
        proxy_pass http://{{cUpstreamServerName}};
    }

    location /longpolling {
        proxy_pass http://{{cUpstreamServerNameChat}};
    }
}

server {
    listen 443;
    server_name {{cPublicServerName}};

    # odoo log files
    access_log /var/log/nginx/ssl.access_{{cUpstreamServerName}};
    error_log  /var/log/nginx/ssl.error_{{cUpstreamServerName}};

    # SSL parameters
    ssl on;

    #these are changed to those used by certbot
    ssl_certificate /etc/ssl/certs/ssl-cert-snakeoil.pem;
    ssl_certificate_key /etc/ssl/private/ssl-cert-snakeoil.key;

    ssl_session_timeout 30m;
    ssl_protocols TLSv1 TLSv1.1 TLSv1.2;
    ssl_ciphers 'ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA';
    ssl_prefer_server_ciphers on;
    # End SSL parameters

    # cache some static data in memory for 60mins.
    # under heavy load this should relieve stress on the OpenERP web interface a bit.
    location ~* /web/static/ {
        proxy_cache_valid 200 60m;
        proxy_buffering    on;
        expires 864000;
        proxy_pass http://{{cUpstreamServerName}};
    }

    location / {
        proxy_pass http://{{cUpstreamServerName}};
    }

    location /longpolling {
        proxy_pass http://{{cUpstreamServerNameChat}};
    }

}
