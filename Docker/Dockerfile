FROM alpine
RUN apk add mosquitto
RUN apk add --update nodejs npm
RUN npm install -g nodemon
RUN apk add git
RUN apk add --no-cache --upgrade bash
RUN git clone https://github.com/GregoryIvo/Scale-ingInventoryManagement.git
WORkDIR Scale-ingInventoryManagement/ioFOGMQTTMessenger/
RUN npm install --only=production
ADD start.sh .
CMD ["sh","start.sh"]