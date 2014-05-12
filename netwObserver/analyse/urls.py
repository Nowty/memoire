from django.conf.urls import patterns, url

from analyse import views

urlpatterns = patterns('',
	url(r'^$', views.general),
	url(r'general/', views.general, name='general'),
	
	url(r'wifi/ap', views.wifiAP, name='wifiAP'),
	url(r'wifi/rap', views.wifiRAP, name='wifiRAP'),
	url(r'wifi/users', views.wifiUsers, name='wifiUsers'),
	url(r'wifi/', views.wifiOverview, name='wifi'),

	url(r'controller/', views.controller, name='controller'),
	url(r'dhcp/', views.dhcp, name='dhcp'),
	url(r'radius/', views.radius, name='radius'),
)