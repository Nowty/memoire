from django.db import models
from custom import macField
from datetime import datetime, timedelta
from django.utils import timezone


## Device
class Device(models.Model):
	# If primary key => not inherited
	macAddress = macField.MACAddressField(unique=True)
	ip = models.GenericIPAddressField(null=True)
	lastTouched = models.DateTimeField(null=True)

	class Meta:
		abstract = True
		ordering = ["macAddress"]


## Mobile stations Model
class MobileStation(Device):
	DOT11_PROTOCOLS = (('a',"800.11a"),('b',"800.11b"),('g',"800.11g"),('n2',"800.11n (2.5Ghz)"),('n5',"800.11n (5Ghz)"))
	
	ssid = models.CharField(max_length=25, null=True)
	dot11protocol = models.CharField(max_length=2, choices=DOT11_PROTOCOLS, null=True)
	def __str__(self):
		return self.macAddress


## Access Point Model
class AccessPoint(Device):
	name = models.CharField(max_length=50, null=True)
	location = models.CharField(max_length=50, null=True)
	def __str__(self):
		return self.name + " : " + self.macAdress + ' (' + self.ip + ')'


## User
class User(models.Model):
	login = models.CharField(max_length=128, primary_key=True)
	email = models.EmailField(max_length=254, null=True)
	def __str__(self):
		return self.login

	class Meta:
		ordering = ["login"]



# Log models
## Radius Model
class RadiusEvent(models.Model):
	date = models.DateTimeField()
	server = models.CharField(max_length=25)
	radiusType = models.CharField(max_length=10) #TODO choice
	
	user = models.ForeignKey(User,null=True)
	message = models.CharField(max_length=128,null=True)

	def __str__(self):
		return "" + self.radiusType + " : " +  str(self.login)

	class Meta:
		unique_together = (('date', 'server'),)

## DHCP model
class DHCPEvent(models.Model):
	date = models.DateTimeField()
	server = models.CharField(max_length=5)
	device = models.ForeignKey(MobileStation, null=True)
	dhcpType = models.CharField(max_length=10)
	ip = models.GenericIPAddressField(null=True)
	message = models.CharField(max_length=256, null=True)

## Wism model
class WismEvent(models.Model):
	date = models.DateTimeField()
	wismIp = models.GenericIPAddressField()

	category = models.CharField(max_length=10)
	severity = models.DecimalField(max_digits=1,decimal_places=0)
	mnemo = models.CharField(max_length=56)

	message = models.CharField(max_length=256)

	class Meta:
		unique_together = (('date', 'wismIp'),)


########################################################################

## Error Parsing
class BadLog(models.Model):
	log = models.CharField(max_length=256)
	cause = models.CharField(max_length=256)
	def __str__(self):
		return "" + self.log + " --> " +  self.cause + '\n'

# Tasks model
class CurrentTask(models.Model):
	lastTouched = models.DateTimeField()
	name = models.CharField(max_length=25)
	owner = models.CharField(max_length=25)
	state = models.CharField(max_length=25)
	progress = models.CharField(max_length=4)
 
	def stillActive(self):
		return (timezone.now() - self.lastTouched) < timedelta(minutes=10)

	def __str__(self):
		return self.name + " by " + self.owner + ": " + ( "active" if self.stillActive() else "inactive")


####################

