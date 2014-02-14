
/**
 * Module dependencies.
 */

var express = require('express');
var routes = require('./routes');
var user = require('./routes/user');
var equares = require('./routes/equares');
var http = require('http');
var path = require('path');
var url = require('url');

var mongoose = require('mongoose');
var passport = require('passport');


var env = process.env.NODE_ENV || 'development',
  config = require('./config/config')[env];


mongoose.connect(config.db);
require('./config/user')
require('./config/passport')(passport)

var app = express();

// all environments
app.set('port', process.env.PORT || 3000);
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'jade');

app.use(express.favicon(path.join(__dirname, 'public/images/favicon.ico')));
app.use(express.logger('dev'));
app.use(express.json());
app.use(express.urlencoded());
app.use(express.methodOverride());
app.use(express.cookieParser('your secret here'));
app.use(express.session({secret: "dde9796b-28ae-4b55-9d03-2cecc8d9ead3"}));

app.use(passport.initialize());
app.use(passport.session());

app.use(app.router);
app.use(express.static(path.join(__dirname, 'public')));

// development only
if ('development' == env) {
  app.use(express.errorHandler());
}

app.get('/', routes.index);
app.get('/users', user.list);

equares.bind(app)

app.use(function(err, req, res, next){
  res.status(err.status || 500);
  res.render('500', { error: err });
});

app.use(function(req, res, next){
  res.status(404);
  if (req.accepts('html')) {
    res.render('404', { url: req.url });
    return;
  }
  if (req.accepts('json')) {
    res.send({ error: 'Not found' });
    return;
  }
  res.type('txt').send('Not found');
});

require('./config/routes')(app, passport);

http.createServer(app).listen(app.get('port'), function(){
  console.log('Express server listening on port ' + app.get('port'));
});