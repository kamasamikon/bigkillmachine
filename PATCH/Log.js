/**
 * This is a simple implementation of a logging mechanism that has
 * been abstracted to use XCOM's log methods for SVG, and console.log for
 * HTML. By instantiating this class you get a function back that
 * when called will output the debug to the console in the given
 * module and class contexts. For example, you could log context
 * management using a `moduleContext` of 'core' and `classContext` of
 * 'ContextManager'.
 *
 * @class $N.apps.core.Log
 * @author mbrown
 * @constructor
 * @param moduleContext The module context.
 * @param classContext The class context.
 * @return {Function} See the `log` method below
 */

/*global navigator,console*/

var $N = $N || {};

(function () {
	if (!window.console) { //fix for IE
		window.console = {
			log: function () {}
		};
	}

	var mode = navigator.userAgent.indexOf("Ekioh") > -1 ? "SVG" : "HTML", domAbstraction = {
		SVG: {
			/**
			* Logs a message to the logger.
			* @method log
			* @param {String} method The name of the calling method.
			* @param {String} message The log message.
			* @param {String} type The logging level.
			*
			*		var log = new $N.apps.core.Log("LOG CONTEXT", "APP CONTEXT");
			*		log("Method Name", "Debug Message", "debug");
			*/
			log: function (logContext, appContext, method, message, type) {
				/*global alert: false */

                    alert(logContext + "[" + appContext + "." + method + "] " + message);

				/*global alert: true */
			}
		},
		HTML: {
			//No need to YUIDoc as it is taken care of in the SVG abstraction
			log: function (logContext, appContext, method, message, type) {
				if ($N.apps.log4javascript) {
					$N.apps.log4javascript[type](logContext, "[" + appContext + "." + method + "] " + message);
				} else {
					console.log(logContext + " [" + appContext + "." + method + "] " + message);
				}
			}
		}
	}[mode];

	function Log(moduleContext, classContext) {
		return function (method, message, type) {
			if (!type) {
				type = "debug";
			}

			if (window.XCOM && XCOM.dalog) {
				switch (type) {
				case "error":
					XCOM.dalog('E', "[" + moduleContext + ":" + classContext + "." + method + "] " + message);
					break;

				case "warn":
					XCOM.dalog('W', "[" + moduleContext + ":" + classContext + "." + method + "] " + message);
					break;

				case "info":
					XCOM.dalog('I', "[" + moduleContext + ":" + classContext + "." + method + "] " + message);
					break;

				case "debug":
				default:
					XCOM.dalog('D', "[" + moduleContext + ":" + classContext + "." + method + "] " + message);
					break;
				}
				return;
			}

			if (!$N.apps.core.LogConfig || $N.apps.core.LogConfig.isLogToBeOutput(moduleContext, classContext, method, type)) {
				domAbstraction.log(moduleContext, classContext, method, message, type);
			}
		};
	}

	/**
	 * Constant to denote a logged error message
	 * @property {String} LOG_ERROR
	 * @static
	 * @readonly
	 */
	Log.LOG_ERROR = "error";

	/**
	 * Constant to denote a logged warning message
	 * @property {String} LOG_WARN
	 * @static
	 * @readonly
	 */
	Log.LOG_WARN = "warn";

	/**
	 * Constant to denote a logged debug message
	 * @property {String} LOG_DEBUG
	 * @static
	 * @readonly
	 */
	Log.LOG_DEBUG = "debug";

	/**
	 * Constant to denote a logged information message
	 * @property {String} LOG_INFO
	 * @static
	 * @readonly
	 */
	Log.LOG_INFO = "info";

	$N.apps = $N.apps || {};
	$N.apps.core = $N.apps.core || {};
	$N.apps.core.Log = Log;

}());
