

/**@obj Device.DNS.  */
CWMPObject DNS_Objs[]={
//	{"Client", NULL, NULL, NULL, NULL, DNSClient_Objs, DNSClient_Params, NULL, eObject,  0},
	{"Diagnostics", NULL, NULL, NULL, NULL, Diagnostics_Objs, NULL, NULL, eObject,  0},
	{NULL}
};
/**@endobj Device.DNS.  */


/**@obj X_RJIL_SwitchFeatures_Management VLAN.  */
CPEGETFUNC(get_RJIL_SwitchFeatures_ManagementVLANID);
CPESETFUNC(set_RJIL_SwitchFeatures_ManagementVLANID);
CWMPParam RJIL_SwitchFeatures_Management_VLAN_Params[]={
	{ "ManagementVLANID", get_RJIL_SwitchFeatures_ManagementVLANID, set_RJIL_SwitchFeatures_ManagementVLANID, NULL, RPC_RW, eUnsignedInt, 0, 0},
	{NULL}
};
/**@endobj X_RJIL_SwitchFeatures_Management VLAN.  */



CPEGETFUNC(get_X_RJIL_SwitchFeatures_LinkAggregationGroup_AssociatedPort);
CPEGETFUNC(get_X_RJIL_SwitchFeatures_LinkAggregationGroup_LinkStatusEnable);
CPEGETFUNC(get_X_RJIL_SwitchFeatures_LinkAggregationGroup_ActiveMember);
CPEGETFUNC(get_X_RJIL_SwitchFeatures_LinkAggregationGroup_InactiveMember);
CPESETFUNC(set_X_RJIL_SwitchFeatures_LinkAggregationGroup_AssociatedPort);
CPESETFUNC(set_X_RJIL_SwitchFeatures_LinkAggregationGroup_LinkStatusEnable);
CPESETFUNC(set_X_RJIL_SwitchFeatures_LinkAggregationGroup_ActiveMember);
CPESETFUNC(set_X_RJIL_SwitchFeatures_LinkAggregationGroup_InactiveMember);
CWMPParam X_RJIL_SwitchFeatures_LinkAggregationGroup_Params[]={
	{ "AssociatedPort", get_X_RJIL_SwitchFeatures_LinkAggregationGroup_AssociatedPort, set_X_RJIL_SwitchFeatures_LinkAggregationGroup_AssociatedPort, NULL, RPC_RW, eString, 0, 0},
	{ "LinkStatusEnable", get_X_RJIL_SwitchFeatures_LinkAggregationGroup_LinkStatusEnable, set_X_RJIL_SwitchFeatures_LinkAggregationGroup_LinkStatusEnable, NULL, RPC_RW, eBoolean, 0, 0},
	{ "ActiveMember", get_X_RJIL_SwitchFeatures_LinkAggregationGroup_ActiveMember, set_X_RJIL_SwitchFeatures_LinkAggregationGroup_ActiveMember, NULL, RPC_RW, eString, 0, 0},
	{ "InactiveMember", get_X_RJIL_SwitchFeatures_LinkAggregationGroup_InactiveMember, set_X_RJIL_SwitchFeatures_LinkAggregationGroup_InactiveMember, NULL, RPC_RW, eString, 0, 0},
	{NULL}
};


/**@obj Device.  */
CPEADDOBJ(initIP);
CPEADDOBJ(initManagementServer);
CPECOMMIT(commitManagementServer);
CPEADDOBJ(addX_RJIL_SwitchFeatures_LinkAggregationGroup);
CPECOMMIT(commitX_RJIL_SwitchFeatures_LinkAggregationGroup);
CWMPObject Device_Objs[]={
	{"DNS", NULL, NULL, NULL, NULL, DNS_Objs, NULL, NULL, eObject,  0},
	{"X_RJIL_SwitchFeatures_LinkAggregationGroup", NULL, addX_RJIL_SwitchFeatures_LinkAggregationGroup, commitX_RJIL_SwitchFeatures_LinkAggregationGroup, NULL, NULL, X_RJIL_SwitchFeatures_LinkAggregationGroup_Params, NULL, eStaticInstance,  0},
	{"X_RJIL_SwitchFeatures_ManagementVLAN", NULL, NULL, NULL, NULL, NULL, RJIL_SwitchFeatures_Management_VLAN_Params, NULL, eObject, 0},

	{NULL}
};

/**@endobj Device.  */
/** CWMP ROOT Object Table  */

CWMPObject CWMP_RootObject[]={
	{"Device", NULL, NULL, NULL, NULL, Device_Objs, NULL, NULL, eObject, 0},
	{NULL}
};
