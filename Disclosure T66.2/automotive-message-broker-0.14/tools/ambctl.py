#!/usr/bin/python

import argparse
import dbus
import sys
import json
import fileinput
import termios, fcntl, os
import curses.ascii
import traceback
from gi.repository import GObject, GLib

from dbus.mainloop.glib import DBusGMainLoop

class bcolors:
		HEADER = '\x1b[95m'
		OKBLUE = '\x1b[94m'
		OKGREEN = '\x1b[92m'
		WARNING = '\x1b[93m'
		FAIL = '\x1b[91m'
		ENDC = '\x1b[0m'
		GREEN = '\x1b[32m'
		WHITE = '\x1b[37m'
		BLUE = '\x1b[34m'

class Autocomplete:
	class Cmd:
		name = ""
		description = ""
		def __init__(self, n, d):
			self.name = n
			self.description = d

	commands = []
	properties = []

	def __init__(self):
		self.commands = []
		self.commands.append(Autocomplete.Cmd('help', 'Prints help data (also see COMMAND help)'))
		self.commands.append(Autocomplete.Cmd('list', 'List supported ObjectNames'))
		self.commands.append(Autocomplete.Cmd('get', 'Get properties from an ObjectName'))
		self.commands.append(Autocomplete.Cmd('listen', 'Listen for changes on an ObjectName'))
		self.commands.append(Autocomplete.Cmd('set', 'Set a property for an ObjectName'))
		self.commands.append(Autocomplete.Cmd('getHistory', 'Get logged data within a time range'))
		self.commands.append(Autocomplete.Cmd('plugin', 'enable, disable and get info on a plugin'))
		self.commands.append(Autocomplete.Cmd('quit', 'Exit ambctl'))

		try:
			bus = dbus.SystemBus()
			managerObject = bus.get_object("org.automotive.message.broker", "/");
			managerInterface = dbus.Interface(managerObject, "org.automotive.Manager")
			self.properties = managerInterface.List()
		except dbus.exceptions.DBusException, error:
			print error

	def complete(self, partialString, commandsOnly = False):
		results = []

		sameString = ""

		for cmd in self.commands:
			if not (len(partialString)) or cmd.name.startswith(partialString):
				results.append(cmd.name)

		if not commandsOnly:
			for property in self.properties:
				if not(len(partialString)) or property.startswith(partialString):
					results.append(str(property))

		if len(results) > 1 and len(results[0]) > 0:
			for i in range(len(results[0])):
				for j in range(len(results[0])-i+1):
					if j > len(sameString) and all(results[0][i:i+j] in x for x in results):
						sameString = results[0][i:i+j]
		elif len(results) == 1:
			sameString = results[0]

		return results, sameString


def help():
		help = ("Available commands:\n")
		autocomplete = Autocomplete()
		for cmd in autocomplete.commands:
			help += bcolors.HEADER + cmd.name + bcolors.WHITE
			for i in range(1, 15 - len(cmd.name)):
				help += " "
			help += cmd.description + "\n"

		return help

def changed(interface, properties, invalidated):
	print json.dumps(properties, indent=2)

def listPlugins():
	list = []
	for root, dirs, files in os.walk('@PLUGIN_SEGMENT_INSTALL_PATH@'):
		for file in files:
			fullpath = root + "/" + file;
			pluginFile = open(fullpath)
			try:
				data = json.load(pluginFile)

				data['segmentPath'] = fullpath
				list.append(data)
			except ValueError, e:
				print "error parsing json file", file, ":", e
				traceback.print_stack()
			finally: pluginFile.close()
	return list

def enablePlugin(pluginName, enabled):
	return setPluginProperty(pluginName, "enabled", enabled);

def setPluginProperty(pluginName, key, value):
	list = listPlugins()

	for plugin in list:
		if plugin["name"] == pluginName:
			try :
				if key not in plugin:
					print "Key not found: ", key
					return False
				type = plugin[key].__class__
				if type == bool:
					value = value.lower() == "true"
				value = type(value)
				plugin[key] = value
				file = open(plugin["segmentPath"], 'rw+')
				plugin.pop('segmentPath', None)
				fixedData = json.dumps(plugin, separators=(', ', ' : '), indent=4)
				fixedData = fixedData.replace('    ','\t');
				file.truncate()
				file.write(fixedData)
				file.close()
				return True
			except IOError, error:
				print error
				return False
	return False
class Subscribe:
	subscriptions = {}
def processCommand(command, commandArgs, noMain=True):

	if command == 'help':
		print help()
		return 1


	def getManager():
		try:
			bus = dbus.SystemBus()
			managerObject = bus.get_object("org.automotive.message.broker", "/");
			managerInterface = dbus.Interface(managerObject, "org.automotive.Manager")
			return managerInterface, bus
		except:
			print "Error connecting to AMB.  is AMB running?"
			return None

	if command == "list" :
		managerInterface, bus = getManager()
		if managerInterface == None:
			return 0
		supportedList = managerInterface.List()
		for objectName in supportedList:
			print objectName
		return 1
	elif command == "get":
		if len(commandArgs) == 0:
			commandArgs = ['help']
		if commandArgs[0] == "help":
			print "ObjectName [ObjectName...]"
			return 1
		managerInterface, bus = getManager()
		if managerInterface == None:
			return 0
		for objectName in commandArgs:
			objects = managerInterface.FindObject(objectName)
			print objectName
			for o in objects:
				propertiesInterface = dbus.Interface(bus.get_object("org.automotive.message.broker", o),"org.freedesktop.DBus.Properties")
				print json.dumps(propertiesInterface.GetAll("org.automotive."+objectName), indent=2)
		return 1
	elif command == "listen":
		off = False
		if len(commandArgs) == 0:
			commandArgs = ['help']
		if commandArgs[0] == "help":
			print "[help] [off] ObjectName [ObjectName...]"
			return 1
		elif commandArgs[0] == "off":
			off=True
			commandArgs=commandArgs[1:]
		managerInterface, bus = getManager()
		if managerInterface == None:
			return 1
		for objectName in commandArgs:
			objects = managerInterface.FindObject(objectName)
			for o in objects:
				if off == False:
					signalMatch = bus.add_signal_receiver(changed, dbus_interface="org.freedesktop.DBus.Properties", signal_name="PropertiesChanged", path=o)
					Subscribe.subscriptions[o] = signalMatch
				else:
					try:
						signalMatch = Subscribe.subscriptions.get(o)
						signalMatch.remove()
						del Subscribe.subscriptions[o]
					except KeyError:
						print "not lisenting to object at: ", o
						pass
		if not noMain == True:
			try:
				main_loop = GObject.MainLoop()
				main_loop.run()
			except KeyboardInterrupt:
				return 1
			except:
				traceback.print_stack()
	elif command == "set":
		if len(commandArgs) == 0:
			commandArgs = ['help']
		if len(commandArgs) and commandArgs[0] == "help":
			print "ObjectName PropertyName VALUE [ZONE]"
			return 1
		if len(commandArgs) < 3:
			print "set requires more arguments (see set help)"
			return 1
		objectName = commandArgs[0]
		propertyName = commandArgs[1]
		value = commandArgs[2]
		zone = 0
		if len(commandArgs) == 4:
			zone = int(commandArgs[3])
		managerInterface, bus = getManager()
		if managerInterface == None:
			return 1
		object = managerInterface.FindObjectForZone(objectName, zone)
		propertiesInterface = dbus.Interface(bus.get_object("org.automotive.message.broker", object),"org.freedesktop.DBus.Properties")
		property = propertiesInterface.Get("org.automotive."+objectName, propertyName)
		if property.__class__ == dbus.Boolean:
			value = value.lower() == "true"
		realValue = property.__class__(value)
		propertiesInterface.Set("org.automotive."+objectName, propertyName, realValue)
		property = propertiesInterface.Get("org.automotive."+objectName, propertyName)
		if property == realValue:
			print propertyName + " = ", property
		else:
			print "Error setting property.  Expected value: ", realValue, " Received: ", property
		return 1
	elif command == "getHistory":
		if len(commandArgs) == 0:
			commandArgs = ['help']
		if commandArgs[0] == "help":
			print "ObjectName [ZONE] [STARTTIME] [ENDTIME] "
			return 1
		if len(commandArgs) < 1:
			print "getHistory requires more arguments (see getHistory help)"
			return 1
		objectName = commandArgs[0]
		start = 1
		if len(commandArgs) >= 3:
			start = float(commandArgs[2])
		end = 9999999999
		if len(commandArgs) >= 4:
			end = float(commandArgs[3])
		zone = 0
		if len(commandArgs) >= 2:
			zone = int(commandArgs[1])
		managerInterface, bus = getManager()
		if managerInterface == None:
			return 1
		object = managerInterface.FindObjectForZone(objectName, zone);
		propertiesInterface = dbus.Interface(bus.get_object("org.automotive.message.broker", object),"org.automotive."+objectName)
		print json.dumps(propertiesInterface.GetHistory(start, end), indent=2)
	elif command == "plugin":
		if len(commandArgs) == 0:
			commandArgs = ['help']
		if commandArgs[0] == 'help':
			print "[list] [pluginName] [key value]"
			return 1
		elif commandArgs[0] == 'list':
			for plugin in listPlugins():
				print plugin['name']
			return 1
		elif len(commandArgs) == 1:
			for plugin in listPlugins():
				if plugin['name'] == commandArgs[0]:
					print json.dumps(plugin, indent=4)
					return 1
				else:
					print "name: " + plugin['name'] + "==?" + commandArgs[0]
			print "plugin not found: ", commandArgs[0]
			return 0
		else:
			if len(commandArgs) < 3:
				return 1
			plugin = commandArgs[0]
			key = commandArgs[1]
			value = commandArgs[2]

			if not setPluginProperty(plugin, key, value):
				print "Could not set property"
			print plugin, key, ":", value
			return 1

	else:
		print "Invalid command"
	return 1



parser = argparse.ArgumentParser(prog="ambctl", description='Automotive Message Broker DBus client tool', add_help=False)
parser.add_argument('command', metavar='COMMAND [help]', nargs='?', default='stdin', help='amb dbus command')
parser.add_argument('commandArgs', metavar='ARG', nargs='*',
			help='amb dbus command arguments')
parser.add_argument('-h', '--help', help='print help', action='store_true')

args = parser.parse_args()

if args.help:
		parser.print_help()
		print
		print help()
		sys.exit()

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

if args.command == "stdin":
		class Data:
				history = []
				line = ""
				templine = ""
				promptAmbctl = "[ambctl]"
				promptEnd = "# "
				fullprompt = promptAmbctl + promptEnd
				curpos = 0
				historypos = -1
				autocomplete = Autocomplete()
				def full_line_len(self):
						return len(self.fullprompt) + len(self.line)
				def insert(self, str):
						if self.curpos == len(self.line):
								self.line+=str
								self.curpos = len(self.line)
						else:
								self.line = self.line[:self.curpos] + str + self.line[self.curpos:]
								self.curpos+=1
				def arrow_back(self):
						if self.curpos > 0:
								self.curpos-=1
								return True
						return False

				def arrow_forward(self):
						if self.curpos < len(self.line):
								self.curpos+=1
								return True
						return False

				def back_space(self):
						if self.curpos > 0:
								self.curpos-=1
								self.line = self.line[:self.curpos] + self.line[self.curpos+1:]
								return True
						return False
				def delete(self):
						if self.curpos < len(self.line):
								self.line = self.line[:self.curpos] + self.line[self.curpos+2:]
								return True
						return False

				def save_temp(self):
						if len(self.history)-1 == 0 or len(self.history)-1 != self.historypos:
								return
						self.templine = self.line

				def push(self):
						self.history.append(self.line)
						self.historypos = len(self.history)-1
						self.clear()

				def set(self, str):
						self.line = str
						self.curpos = len(self.line)

				def history_up(self):
						if self.historypos >= 0:
								self.line = self.history[self.historypos]
								if self.historypos != 0:
										self.historypos-=1
								return True
						return False

				def history_down(self):
						if self.historypos >= 0 and self.historypos < len(self.history)-1:
								self.historypos+=1
								self.line = self.history[self.historypos]

						else:
								self.historypos = len(self.history)-1
								self.set(self.templine)

						return True

				def clear(self):
						self.set("")
						templist = ""

		def erase_line():
				sys.stdout.write('\x1b[2K\x1b[80D')

		def cursor_left():
				sys.stdout.write('\x1b[1D')

		def cursor_right():
				sys.stdout.write('\x1b[1C')

		def display_prompt():
				sys.stdout.write(bcolors.OKBLUE+Data.promptAmbctl+bcolors.WHITE+Data.promptEnd);

		def redraw(data):
				erase_line()
				display_prompt()
				sys.stdout.write(data.line)
				cursorpos = len(data.line) - data.curpos
				for x in xrange(cursorpos):
						cursor_left()
				sys.stdout.flush()

		def handle_keyboard(source, cond, data):
						str = source.read()
						#print "char: ", ord(str)

						if len(str) > 1:
							if ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 68:
								#left arrow
								if data.arrow_back():
									cursor_left()
									sys.stdout.flush()
							elif ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 67:
								#right arrow
								if data.arrow_forward():
									cursor_right()
									sys.stdout.flush()
							elif ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 70:
								#end
								while data.arrow_forward():
									cursor_right()
								sys.stdout.flush()
							elif ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 72: #home
								while data.arrow_back():
									cursor_left()
								sys.stdout.flush()
							elif len(str) == 4 and ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 51 and ord(str[3]) == 126:
								#del
								data.delete()
								redraw(data)
							elif ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 65:
								#up arrow
								data.save_temp()
								data.history_up()
								while data.arrow_forward():
									cursor_right()
								redraw(data)
							elif ord(str[0]) == 27 and ord(str[1]) == 91 and ord(str[2]) == 66:
								#down arrow
								data.history_down()
								while data.arrow_forward():
									cursor_right()
								redraw(data)
						elif ord(str) == 10:
							#enter
							if data.line == "":
								return True
							print ""
							words = data.line.split(' ')
							if words[0] == "quit":
								termios.tcsetattr(fd, termios.TCSAFLUSH, old)
								fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)
								sys.exit()
							try:
								if len(words) > 1:
									processCommand(words[0], words[1:])
								else:
									processCommand(words[0], [])
							except dbus.exceptions.DBusException, error:
								print error
							data.push();
							data.clear()
							redraw(data)
						elif ord(str) == 127: #backspace
							data.back_space()
							redraw(data)
						elif ord(str) == 9:
							#tab
							#get last string:
							wordsList = data.line.split(' ')
							toComplete = wordsList[-1]
							results, samestring = data.autocomplete.complete(toComplete)
							if len(samestring) and len(samestring) > len(toComplete) and not (samestring == toComplete):
								if len(wordsList) > 1:
									data.line = ' '.join(wordsList[0:-1]) + ' ' + samestring
								else:
									data.line = samestring
								while data.arrow_forward():
									cursor_right()

							elif len(results) and not results[0] == toComplete:
								print ''
								if len(results) <= 3:
									print ' '.join(results)
								else:
									longestLen = 0
									for r in results:
										if len(r) > longestLen:
											longestLen = len(r)
									i=0
									while i < len(results):
										row = ""
										numCols = 3
										if len(results) < i+3:
											numCols = len(results) - i
										for n in xrange(numCols):
											row += results[i]
											for n in xrange((longestLen + 5) - len(results[i])):
												row += ' '
											i = i + 1
										print row

							redraw(data)
						elif curses.ascii.isprint(ord(str)) or ord(str) == curses.ascii.SP: #regular text
							data.insert(str)
							redraw(data)

						return True
		print "@PROJECT_PRETTY_NAME@ @PROJECT_VERSION@"

		data = Data()
		fd = sys.stdin.fileno()
		old = termios.tcgetattr(fd)
		new = termios.tcgetattr(fd)
		new[3] = new[3] & ~termios.ICANON & ~termios.ECHO
		termios.tcsetattr(fd, termios.TCSANOW, new)

		oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
		fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

		io_stdin = GLib.IOChannel(fd)
		io_stdin.add_watch(GLib.IO_IN, handle_keyboard, data)

		try:
			erase_line()
			display_prompt()
			sys.stdout.flush()
			main_loop = GObject.MainLoop()
			main_loop.run()
		except KeyboardInterrupt:
			sys.exit()
		finally:
			termios.tcsetattr(fd, termios.TCSAFLUSH, old)
			fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)
			print ""
			sys.exit()

else:
	try:
		processCommand(args.command, args.commandArgs, False)
	except dbus.exceptions.DBusException, error:
		print error
