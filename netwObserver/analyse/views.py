from django.shortcuts import render
from analyse.computation import aggregator
from gatherer.models import AccessPoint

from datetime import datetime

# Create your views here.
def general(request):
	context = {}
	context['app'] = 'analysis'
	context['cat'] = 'gen'
	
	context['nbrUsers'] = aggregator.getNbrOfUsers()
	context['nbrAP'] = aggregator.getNbrOfAP()

	return render(request, "analyse/general.html", context)

def controller(request):
	context = {}
	context["cat"] = 'controller'
	context['app'] = 'analysis'

	context['byCategory'] = aggregator.getWismLogsByCategory()

	return render(request, "analyse/controller.html", context)


def wifiOverview(request):
	context= {}
	context['app'] = 'analysis'
	context['cat'] = 'wifi'
	context['section'] = 'overview'

	return render(request, "analyse/wifiOverview.html", context)

def wifiAP(request):
	context= {}
	context['app'] = 'analysis'
	context['cat'] = 'wifi'
	context['section'] = 'ap'

	context["allAP"] = AccessPoint.objects.all().order_by('name')

	if request.method == 'POST' and 'selectedAP' in request.POST:
		context["ap"] = AccessPoint.objects.get(id=int(request.POST['selectedAP']))
		context["apBandwidth"] = aggregator.getAPData(context["ap"])
		#context["interfaceData"] = aggregator.getIfData(context["ap"])

	return render(request, "analyse/wifiAP.html", context)

def wifiRAP(request):
	context= {}
	context['app'] = 'analysis'
	context['cat'] = 'wifi'
	context['section'] = 'rap'

	return render(request, "analyse/wifiRAP.html", context)

def wifiUsers(request):
	context= {}
	context['app'] = 'analysis'
	context['cat'] = 'wifi'
	context['section'] = 'users'

	context["proto"] = aggregator.getUsersByDot11Protocol()
	context["ssid"] = aggregator.getUsersBySSID()

	return render(request, "analyse/wifiUsers.html", context)


def dhcp(request):
	context= {}
	context['app'] = 'analysis'
	context['cat'] = 'dhcp'

	context['byType'] = aggregator.getDhcpLogByType()

	return render(request, "analyse/dhcp.html", context)


def radius(request):
	context= {}
	context['app'] = 'analysis'
	context['cat'] = 'radius'

	context['successRate'] = aggregator.getRadiusSuccessRate()

	return render(request, "analyse/radius.html", context)
