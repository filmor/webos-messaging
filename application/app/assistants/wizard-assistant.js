var WizardAssistant = function (params) {
	this.params = params;
	
	//Set Login Details
	this.params.username = "";
	this.params.password = "";
	
	//Set Account Options
	this.params.displayAlias = this.params.initialTemplate.loc_AccountOptions.Alias.Show;
	this.params.displayAvatar = this.params.initialTemplate.loc_AccountOptions.Avatar.Show;
	this.params.displayBadCert = this.params.initialTemplate.loc_AccountOptions.BadCert.Show;
	this.params.EnableAlias = this.params.initialTemplate.loc_AccountOptions.Alias.Value;
	this.params.EnableAvatar = this.params.initialTemplate.loc_AccountOptions.Avatar.Value;
	this.params.EnableBadCert = this.params.initialTemplate.loc_AccountOptions.BadCert.Value;
	
	//Used by functions below
	self = this;
};
    
WizardAssistant.prototype.setup = function() {
	//Setup toggles
	this.toggleattr = {
		trueLabel:  this.params.initialTemplate.loc_Toggles.DisplayYes,
		falseLabel:  this.params.initialTemplate.loc_Toggles.DisplayNo
	};

	//Header
	this.controller.get('header').innerHTML = this.params.initialTemplate.loc_name;
	this.controller.get('loc_subheader').innerHTML = this.params.initialTemplate.loc_subheader;
	var templateIconArray = new Array();
	templateIconArray = this.params.initialTemplate.icon.loc_48x48.split('/');
	var bgCss = "url(" + Mojo.appPath + "images/" + templateIconArray[templateIconArray.length - 1] + ") center center no-repeat";
	this.controller.get('title-icon').style.background = bgCss;

	//Email Address
	this.controller.get('loc_usernameLabel').innerHTML = this.params.initialTemplate.loc_usernameLabel;
	this.controller.get('loc_usernameExampleLabel').innerHTML = this.params.initialTemplate.loc_explainLabel;
	var usernameAttributes = {textFieldName: "loc_usernameLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase};
	this.usernameModel = {original: this.params.username || '',disabled: false};
	this.controller.setupWidget('username', usernameAttributes, this.usernameModel);

	//Password Field
	this.controller.get('loc_passwordLabel').innerHTML = this.params.initialTemplate.loc_passwordLabel;
	var passwordAttributes = {textFieldName: "loc_passwordLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false};
	this.passwordModel = {original: this.params.password || '',disabled: false};
	this.controller.setupWidget('password', passwordAttributes, this.passwordModel);

	//Server Options
	//Server Name
	this.controller.get('loc_ServerSetup').innerHTML = this.params.initialTemplate.loc_ServerSetup.Display;
	this.controller.get('loc_servernameLabel').innerHTML = this.params.initialTemplate.loc_ServerSetup.ServerName.Display;
	this.params.servername = this.params.initialTemplate.loc_ServerSetup.ServerName.Value;
	var servernameAttributes = {textFieldName: "loc_servernameLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase};
	this.servernameModel = {original: this.params.servername || '',disabled: false};
	this.controller.setupWidget('server-name', servernameAttributes , this.servernameModel);
	//Server Port
	this.controller.get('loc_serverportLabel').innerHTML = this.params.initialTemplate.loc_ServerSetup.ServerPort.Display;
	this.params.serverport = this.params.initialTemplate.loc_ServerSetup.ServerPort.Value;
	var serverportAttributes = {textFieldName: "loc_serverportLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase,modifierState: Mojo.Widget.numLock, charsAllow: this.TextBoxNumbers.bind(this)};
	this.serverportModel = {original: this.params.serverport || '',disabled: false};
	this.controller.setupWidget('server-port', serverportAttributes , this.serverportModel);
	//Server TLS
	this.controller.get('loc_servertlsLabel').innerHTML = this.params.initialTemplate.loc_ServerSetup.ServerTLS.Display;
	this.controller.get('loc_servertlsExplainLabel').innerHTML = this.params.initialTemplate.loc_ServerSetup.ServerTLS.Explain;
	this.servertlstoggleModel = {value: this.params.initialTemplate.loc_ServerSetup.ServerTLS.Value, disabled: false};
	this.controller.setupWidget('server-tls', this.toggleattr, this.servertlstoggleModel );
	//Server Timeout
	this.timeoutOptions = {
		multiline: true,
		label: $L(this.params.initialTemplate.loc_ServerSetup.ServerTimeout),
		choices: [
			{label: $L('30'), value: 30},
			{label: $L('45'), value: 45},
			{label: $L('60'), value: 60},
			{label: $L('75'), value: 75},
			{label: $L('90'), value: 90},
			{label: $L('105'), value: 105},
			{label: $L('120'), value: 120}
		]
	};
	this.servertimeoutModel = {value: 45, disabled: false};
	this.controller.setupWidget('edit-login-timeout', this.timeoutOptions, this.servertimeoutModel);
	//Buddy List Timeout
	this.buddylistOptions = {
		multiline: true,
		label: $L(this.params.initialTemplate.loc_ServerSetup.ServerBuddyList),
		choices: [
			{label: $L('10'), value: 10},
			{label: $L('20'), value: 20},
			{label: $L('30'), value: 30},
			{label: $L('40'), value: 40},
			{label: $L('50'), value: 50},
			{label: $L('60'), value: 60}
		]
	};
	this.buddylistModel = {value: 10, disabled: false};
	this.controller.setupWidget('edit-buddylist-timeout', this.buddylistOptions, this.buddylistModel);
	
	//Warning
	this.controller.get('loc_ServerSetupWarning').innerHTML = this.params.initialTemplate.loc_ServerSetup.Warning;
	
	//Account Setup
	ShowAccountSetup = false;
	this.controller.get('loc_jabberresourceExampleLabel').hide();
	this.controller.get('loc_xfireversionExampleLabel').hide();
	this.controller.get('loc_sipeloginExampleLabel').hide();
	this.controller.get('loc_sipeuseragentExampleLabel').hide();
		
	//Sametime?
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sametime")
	{
		ShowAccountSetup = true;
		this.controller.get('sametimeoption').show();

		this.controller.get('loc_sametimehideidLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.HideClientID.Display;
		this.controller.get('loc_sametimehideidExplainLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.HideClientID.Explain;
		this.SametimetoggleModel = {value: this.params.initialTemplate.loc_AccountSetup.HideClientID.Value ,disabled: false};
		this.controller.setupWidget('sametime-hideid', this.toggleattr, this.SametimetoggleModel );
	}
	//Jabber?
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.jabber")
	{
		ShowAccountSetup = true;
		this.controller.get('jabberoption').show();
		this.controller.get('loc_jabberresourceExampleLabel').show();

		this.controller.get('loc_jabberresourceLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.JabberResource.Display;
		this.controller.get('loc_jabberresourceExampleLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.JabberResource.Explain;
		var jabberresourceAttributes = {textFieldName: "loc_jabberresourceLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase}
		this.jabberresourceModel = {original: this.params.initialTemplate.loc_AccountSetup.JabberResource.Value || '',disabled: false};
		this.controller.setupWidget('jabber-resource', jabberresourceAttributes , this.jabberresourceModel);
	}
	//XFire?
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.xfire")
	{
		ShowAccountSetup = true;
		this.controller.get('xfireoption').show();
		this.controller.get('loc_xfireversionExampleLabel').show();

		this.controller.get('loc_xfireversionLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.XFireVersion.Display;
		this.controller.get('loc_xfireversionExampleLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.XFireVersion.Explain;
		var xfireversionAttributes = {textFieldName: "loc_xfireversionLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase,modifierState: Mojo.Widget.numLock, charsAllow: this.TextBoxNumbers.bind(this)}
		this.xfireversionModel = {original: this.params.initialTemplate.loc_AccountSetup.XFireVersion.Value || '',disabled: false};
		this.controller.setupWidget('xfire-version', xfireversionAttributes , this.xfireversionModel);
	}
	//SIPE?
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sipe")
	{
		ShowAccountSetup = true;
		this.controller.get('sipeoption1').show();
		this.controller.get('sipeoption2').show();
		this.controller.get('sipeoption3').show();
		this.controller.get('loc_sipeloginExampleLabel').show();
		this.controller.get('loc_sipeuseragentExampleLabel').show();
		
		//Login
		this.controller.get('loc_sipeloginLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.Login.Display;
		this.controller.get('loc_sipeloginExampleLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.Login.Explain;
		var sipeloginAttributes = {textFieldName: "loc_jabberresourceLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase}
		this.sipeloginModel = {original: this.params.initialTemplate.loc_AccountSetup.Login.Value || '',disabled: false};
		this.controller.setupWidget('sipe-login', sipeloginAttributes , this.sipeloginModel);
		
		//User Agent
		this.controller.get('loc_sipeuseragentLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.UserAgent.Display;
		this.controller.get('loc_sipeuseragentExampleLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.UserAgent.Explain;
		var sipeuseragentAttributes = {textFieldName: "loc_jabberresourceLabel",hintText: '',modelProperty: 'original',multiline: false,focus: false,textReplacement: false,textCase: Mojo.Widget.steModeLowerCase}
		this.sipeuseragentModel = {original: this.params.initialTemplate.loc_AccountSetup.UserAgent.Value || '',disabled: false};
		this.controller.setupWidget('sipe-useragent', sipeuseragentAttributes , this.sipeuseragentModel);
		
		//Proxy
		this.controller.get('loc_sipeproxyLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.Proxy.Display;
		this.controller.get('loc_sipeproxyExplainLabel').innerHTML = this.params.initialTemplate.loc_AccountSetup.Proxy.Explain;
		this.sipeproxytoggleModel = {value: this.params.initialTemplate.loc_AccountSetup.Proxy.Value ,disabled: false};
		this.controller.setupWidget('sipe-proxy', this.toggleattr, this.sipeproxytoggleModel );
	}

	if (ShowAccountSetup == true)
	{
		this.controller.get('loc_AccountSetupGroup').show();
		
		//Title
		this.controller.get('loc_AccountSetup').innerHTML = this.params.initialTemplate.loc_AccountSetup.Display;
		
		//Warning
		this.controller.get('loc_AccountSetupWarning').innerHTML = this.params.initialTemplate.loc_AccountSetup.Warning;
	}

	//Account Options
	this.controller.get('loc_AccountOptions').innerHTML = this.params.initialTemplate.loc_AccountOptions.Display;
	//Show or hide account options?
	if ((this.params.displayAlias === false) &&	(this.params.displayAvatar === false) && (this.params.displayBadCert === false)) 
	{
			this.controller.get('AccountOptions').hide();
	}
	else
	{
		this.AvatartoggleModel = {value: this.params.EnableAvatar,disabled: false};
		this.AliastoggleModel = {value: this.params.EnableAlias,disabled: false};
		this.BadCerttoggleModel = {value: this.params.EnableBadCert,disabled: false};
		
		//Hide Alias?
		if (this.params.displayAlias === false)
		{
			this.controller.get('Alias').hide();
		}
		else
		{
			this.controller.get('loc_aliasLabel').innerHTML = this.params.initialTemplate.loc_AccountOptions.Alias.Display;
			this.controller.get('loc_aliasExplainLabel').innerHTML = this.params.initialTemplate.loc_AccountOptions.Alias.Explain;
			this.controller.setupWidget('AliasToggle', this.toggleattr, this.AliastoggleModel );
		}
		//Hide Avatar
		if (this.params.displayAvatar === false)
		{
			this.controller.get('Avatar').hide();
		}
		else
		{
			this.controller.get('loc_avatarLabel').innerHTML = this.params.initialTemplate.loc_AccountOptions.Avatar.Display;
			this.controller.get('loc_avatarExplainLabel').innerHTML = this.params.initialTemplate.loc_AccountOptions.Avatar.Explain;
			this.controller.setupWidget('AvatarToggle', this.toggleattr, this.AvatartoggleModel );
		}
		//Hide Bad Cert
		if (this.params.displayBadCert === false)
		{
			this.controller.get('BadCert').hide();
		}
		else
		{
			this.controller.get('loc_badcertLabel').innerHTML = this.params.initialTemplate.loc_AccountOptions.BadCert.Display;
			this.controller.get('loc_badcertExplainLabel').innerHTML = this.params.initialTemplate.loc_AccountOptions.BadCert.Explain;
			this.controller.setupWidget('BadCertToggle', this.toggleattr, this.BadCerttoggleModel );
		}
		//Warning
		this.controller.get('loc_AccountOptionsWarning').innerHTML = this.params.initialTemplate.loc_AccountOptions.Warning;
	}

	//Setup Sign-In Button
	this.signInButtonModel = {buttonClass:'primary', buttonLabel:$L(this.params.initialTemplate.loc_signInButton), disabled:false};
	this.controller.setupWidget('create-account-button', {type: Mojo.Widget.activityButton}, this.signInButtonModel);
	this.controller.listen('create-account-button', Mojo.Event.tap, WizardAssistant.createAccount.bind(this));
	WizardAssistant.signInButton = this.controller.get('create-account-button');
	WizardAssistant.signInButton.innerHTML = this.params.initialTemplate.loc_signInButton;
	
	//Setup Donate Button
	this.controller.get('loc_Donate').innerHTML = this.params.initialTemplate.loc_Donate;
	Mojo.Event.listen(this.controller.get('DonateButton'), Mojo.Event.tap, this.LoadDonateWeb.bind(this));
};

WizardAssistant.createAccount = function(event) {
	//Check a username was entered
	if (this.usernameModel.original === '')
	{
		this.ShowLoginError ("Validation", "Please enter a valid username");
		return;
	}
	//Check a password was entered
	if (this.passwordModel.original === '')
	{
		this.ShowLoginError ("Validation", "Please enter a valid password");
		return;
	}
	//Check a server name was entered
	if ((this.servernameModel.original === '') && (this.params.initialTemplate.templateId != "org.webosinternals.messaging.sipe"))
	{
		this.ShowLoginError ("Validation", "Please enter a valid server name");
		return;
	}
	//Check a server port was entered
	if (this.serverportModel.original === '')
	{
		this.ShowLoginError ("Validation", "Please enter a valid server port");
		return;
	}
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.xfire")
	{
		//Check a XFire version was entered
		if (this.xfireversionModel.original === '')
		{
			this.ShowLoginError ("Validation", "Please enter a valid XFire version");
			return;
		}
	}
	
	//Disable all controls
	this.DisableControls()
	
	var Template = this.params.initialTemplate
	var Validator = this.params.initialTemplate.validator

	Mojo.Log.info("WizardAssistant Validator=", JSON.stringify(Validator));
	Mojo.Log.info("WizardAssistant Address=", Validator.address);

	//Delete any existing preferences
	//luna-send -i palm://com.palm.db/del {\"query\":{\"from\":\"org.webosinternals.messaging.prefs:1\",\"where\":[{\"prop\":\"templateId\",\"op\":\"=\",\"val\":\"org.webosinternals.messaging.live\"}]}}
	Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'del',{"query":{"from": "org.webosinternals.messaging.prefs:1", "where":[{"prop":"templateId", "op":"=", "val":Template.templateId},{"prop":"UserName", "op":"=", "val":this.usernameModel.original}]}}).then(function(fut) {
	});
	
	//Save standard preferences
	if ((this.params.initialTemplate.templateId != "org.webosinternals.messaging.jabber") && (this.params.initialTemplate.templateId != "org.webosinternals.messaging.sametime") && (this.params.initialTemplate.templateId != "org.webosinternals.messaging.sipe") && (this.params.initialTemplate.templateId != "org.webosinternals.messaging.xfire"))
	{
		//Save account preferences in palm DB
		Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'put',{"objects":[{"_kind":"org.webosinternals.messaging.prefs:1","templateId":Template.templateId,"UserName":this.usernameModel.original,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"EnableAvatar":this.AvatartoggleModel.value,"EnableAlias":this.AliastoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"LoginTimeOut":this.servertimeoutModel.value,"BuddyListTimeOut":this.buddylistModel.value}]}).then(function(fut) {
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant DBPut Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				self.ShowLoginError ("Preferences", "Unable to save preferences!");
				return;
			}
		});
		
		//Set preferences for the IM Account Validator
		Foundations.Comms.PalmCall.call('palm://org.webosinternals.imaccountvalidator/', 'setpreferences',{"templateId":Template.templateId,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"BadCert":this.BadCerttoggleModel.value}).then(function(fut){
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant SetPrefs Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

				self.controller.showAlertDialog({
					title: $L('Set Preferences Failed'),
					message: JSON.stringify(response.errorText),
					choices: [{label:$L("OK"), value:"OK"}]
				});

				//Enable all controls
				this.EnableControls()
				return;
			}
		});
	}
	
	//Save jabber preferences
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.jabber")
	{
		//Save account preferences in palm DB
		Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'put',{"objects":[{"_kind":"org.webosinternals.messaging.prefs:1","templateId":Template.templateId,"UserName":this.usernameModel.original,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"EnableAvatar":this.AvatartoggleModel.value,"EnableAlias":this.AliastoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"JabberResource": this.jabberresourceModel.original,"LoginTimeOut":this.servertimeoutModel.value,"BuddyListTimeOut":this.buddylistModel.value}]}).then(function(fut) {
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant DBPut Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				self.ShowLoginError ("Preferences", "Unable to save preferences!");
				return;
			}
		});
		
		//Set preferences for the IM Account Validator
		Foundations.Comms.PalmCall.call('palm://org.webosinternals.imaccountvalidator/', 'setpreferences',{"templateId":Template.templateId,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"JabberResource": this.jabberresourceModel.original}).then(function(fut){
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant SetPrefs Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

				self.controller.showAlertDialog({
					title: $L('Set Preferences Failed'),
					message: JSON.stringify(response.errorText),
					choices: [{label:$L("OK"), value:"OK"}]
				});

				//Enable all controls
				this.EnableControls()
				return;
			}
		});
	}
	
	//Save sametime preferences
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sametime")
	{
		//Save account preferences in palm DB
		Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'put',{"objects":[{"_kind":"org.webosinternals.messaging.prefs:1","templateId":Template.templateId,"UserName":this.usernameModel.original,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"EnableAvatar":this.AvatartoggleModel.value,"EnableAlias":this.AliastoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"SametimehideID": this.SametimetoggleModel.original,"LoginTimeOut":this.servertimeoutModel.value,"BuddyListTimeOut":this.buddylistModel.value}]}).then(function(fut) {
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant DBPut Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				self.ShowLoginError ("Preferences", "Unable to save preferences!");
				return;
			}
		});
		
		//Set preferences for the IM Account Validator
		Foundations.Comms.PalmCall.call('palm://org.webosinternals.imaccountvalidator/', 'setpreferences',{"templateId":Template.templateId,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"SametimehideID": this.SametimetoggleModel.value}).then(function(fut){
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant SetPrefs Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

				self.controller.showAlertDialog({
					title: $L('Set Preferences Failed'),
					message: JSON.stringify(response.errorText),
					choices: [{label:$L("OK"), value:"OK"}]
				});

				//Enable all controls
				this.EnableControls()
				return;
			}
		});
	}
	
	//Save OCS preferences
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sipe")
	{
		//Save account preferences in palm DB
		Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'put',{"objects":[{"_kind":"org.webosinternals.messaging.prefs:1","templateId":Template.templateId,"UserName":this.usernameModel.original,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"EnableAvatar":this.AvatartoggleModel.value,"EnableAlias":this.AliastoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"SIPEServerLogin": this.sipeloginModel.original,"SIPEUserAgent": this.sipeuseragentModel.original,"SIPEServerProxy": this.sipeproxytoggleModel.value,"LoginTimeOut":this.servertimeoutModel.value,"BuddyListTimeOut":this.buddylistModel.value}]}).then(function(fut) {
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant DBPut Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				self.ShowLoginError ("Preferences", "Unable to save preferences!");
				return;
			}
		});
		
		//Set preferences for the IM Account Validator
		Foundations.Comms.PalmCall.call('palm://org.webosinternals.imaccountvalidator/', 'setpreferences',{"templateId":Template.templateId,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"SIPEServerLogin": this.sipeloginModel.original,"SIPEUserAgent": this.sipeuseragentModel.original,"SIPEServerProxy": this.sipeproxytoggleModel.value}).then(function(fut){
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant SetPrefs Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

				self.controller.showAlertDialog({
					title: $L('Set Preferences Failed'),
					message: JSON.stringify(response.errorText),
					choices: [{label:$L("OK"), value:"OK"}]
				});

				//Enable all controls
				this.EnableControls()
				return;
			}
		});
	}
	
	//Save XFire preferences
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.xfire")
	{
		//Save account preferences in palm DB
		Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'put',{"objects":[{"_kind":"org.webosinternals.messaging.prefs:1","templateId":Template.templateId,"UserName":this.usernameModel.original,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"EnableAvatar":this.AvatartoggleModel.value,"EnableAlias":this.AliastoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"XFireversion": this.xfireversionModel.original,"LoginTimeOut":this.servertimeoutModel.value,"BuddyListTimeOut":this.buddylistModel.value}]}).then(function(fut) {
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant DBPut Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				self.ShowLoginError ("Preferences", "Unable to save preferences!");
				return;
			}
		});
		
		//Set preferences for the IM Account Validator
		Foundations.Comms.PalmCall.call('palm://org.webosinternals.imaccountvalidator/', 'setpreferences',{"templateId":Template.templateId,"ServerName":this.servernameModel.original,"ServerPort":this.serverportModel.original,"ServerTLS":this.servertlstoggleModel.value,"BadCert":this.BadCerttoggleModel.value,"XFireversion": this.xfireversionModel.original}).then(function(fut){
			var results = fut._result.result;
			
			Mojo.Log.info("WizardAssistant SetPrefs Results=", JSON.stringify(results));
			
			if (!results.returnValue === true) {
				Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

				self.controller.showAlertDialog({
					title: $L('Set Preferences Failed'),
					message: JSON.stringify(response.errorText),
					choices: [{label:$L("OK"), value:"OK"}]
				});

				//Enable all controls
				this.EnableControls()
				return;
			}
		});
	}

	//Load the preferences in IMLibpurpleService for later on
	Foundations.Comms.PalmCall.call('palm://org.webosinternals.imlibpurple/', 'loadpreferences',{"templateId":Template.templateId,"UserName":this.usernameModel.original}).then(function(fut){
		var results = fut._result.result;
		
		Mojo.Log.info("WizardAssistant LoadPrefs Results=", JSON.stringify(results));
		
		if (!results.returnValue === true) {
			Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

			self.controller.showAlertDialog({
				title: $L('Load Preferences Failed'),
				message: JSON.stringify(response.errorText),
				choices: [{label:$L("OK"), value:"OK"}]
			});

			//Enable all controls
			this.EnableControls()
			return;
		}
	});

	//Perform the login
	new Mojo.Service.Request(Validator.address, {
		parameters: {
			"username": this.usernameModel.original,
			"password": this.passwordModel.original,
			"templateId": Template.templateId
		},
		onSuccess: this.LoginSuccess.bind(this),
		onFailure: this.LoginFailure.bind(this),
		onComplete: this.LoginComplete.bind(this),
		onError: this.LoginFailure.bind(this)
	});
};

WizardAssistant.prototype.ShowLoginError = function(ErrorTitle, ErrorText) {
	self.controller.showAlertDialog({
		title: $L(ErrorTitle),
		message: JSON.stringify(ErrorText),
		choices: [{label:$L("OK"), value:"OK"}]
	});
	
	//Enable all controls
	this.EnableControls()
	return;
};

WizardAssistant.prototype.LoginComplete = function(response) {
	Mojo.Log.info("Login Complete Response = ", JSON.stringify(response));
};

WizardAssistant.prototype.LoginSuccess = function(response) {
	Mojo.Log.info("Login Success Response = ", JSON.stringify(response));
	
	var Template = self.params.initialTemplate;
	var ResultPassword = response.credentials.common.password;

	//Build Account Settings For PopScene
	this.accountSettings = {};	
	this.accountSettings = {
				"template":Template,
				"username":this.usernameModel.original,
				"defaultResult":{
							"result":{
									returnValue:true,
									"credentials":{
												"common":{
												"password":ResultPassword}
												}
									}
								}
							};

	//Display Account Creation Dialog
	this.popScene();
};

WizardAssistant.prototype.LoginFailure = function(response) {
	Mojo.Log.info("Login Failure Response = ", JSON.stringify(response));

	self.controller.showAlertDialog({
		title: $L('Login Failed'),
		message: JSON.stringify(response.errorText),
		choices: [{label:$L("OK"), value:"OK"}]
	});

	//Remove prefs
	var Template = this.params.initialTemplate

	//Delete any existing preferences
	//luna-send -i palm://com.palm.db/del {\"query\":{\"from\":\"org.webosinternals.messaging.prefs:1\",\"where\":[{\"prop\":\"templateId\",\"op\":\"=\",\"val\":\"org.webosinternals.messaging.live\"}]}}
	Foundations.Comms.PalmCall.call('palm://com.palm.db/', 'del',{"query":{"from": "org.webosinternals.messaging.prefs:1", "where":[{"prop":"templateId", "op":"=", "val":Template.templateId},{"prop":"UserName", "op":"=", "val":this.usernameModel.original}]}}).then(function(fut) {
	});

	//Enable all controls
	this.EnableControls()
	return;
};

WizardAssistant.prototype.DisableControls = function() {
	//Disable Controls
	this.usernameModel.disabled = true;
	this.controller.modelChanged(this.usernameModel);
	this.passwordModel.disabled = true;
	this.controller.modelChanged(this.passwordModel);
	this.signInButtonModel.disabled = true;
 	this.controller.modelChanged(this.signInButtonModel);
 	this.servernameModel.disabled = true;
	this.controller.modelChanged(this.servernameModel);
	this.serverportModel.disabled = true;
	this.controller.modelChanged(this.serverportModel);
	this.servertlstoggleModel.disabled = true;
	this.controller.modelChanged(this.servertlstoggleModel);
	this.servertimeoutModel.disabled = true;
	this.controller.modelChanged(this.servertimeoutModel);
	this.buddylistModel.disabled = true;
	this.controller.modelChanged(this.buddylistModel);
	this.AvatartoggleModel.disabled = true;
	this.controller.modelChanged(this.AvatartoggleModel);
	this.AliastoggleModel.disabled = true;
	this.controller.modelChanged(this.AliastoggleModel);
	this.BadCerttoggleModel.disabled = true;
	this.controller.modelChanged(this.BadCerttoggleModel);
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.jabber")
	{
		this.jabberresourceModel.disabled = true;
		this.controller.modelChanged(this.jabberresourceModel);
	}

	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sametime")
	{
		this.SametimetoggleModel.disabled = true;
		this.controller.modelChanged(this.SametimetoggleModel);
	}

	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sipe")
	{
		this.sipeloginModel.disabled = true;
		this.controller.modelChanged(this.sipeloginModel);
		this.sipeuseragentModel.disabled = true;
		this.controller.modelChanged(this.sipeuseragentModel);
		this.sipeproxytoggleModel.disabled = true;
		this.controller.modelChanged(this.sipeproxytoggleModel);
	}

	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.xfire")
	{
		this.xfireversionModel.disabled = true;
		this.controller.modelChanged(this.xfireversionModel);
	}
};

WizardAssistant.prototype.EnableControls = function() {
	//Disable spinning login button
	WizardAssistant.signInButton.mojo.deactivate();
	
	//Enable other controls
	this.usernameModel.disabled = false;
	this.controller.modelChanged(this.usernameModel);
	this.passwordModel.disabled = false;
	this.controller.modelChanged(this.passwordModel);
	this.signInButtonModel.disabled = false;
 	this.controller.modelChanged(this.signInButtonModel);
 	this.servernameModel.disabled = false;
	this.controller.modelChanged(this.servernameModel);
	this.serverportModel.disabled = false;
	this.controller.modelChanged(this.serverportModel);
	this.servertlstoggleModel.disabled = false;
	this.controller.modelChanged(this.servertlstoggleModel);
	this.servertimeoutModel.disabled = false;
	this.controller.modelChanged(this.servertimeoutModel);
	this.buddylistModel.disabled = false;
	this.controller.modelChanged(this.buddylistModel);
	this.AvatartoggleModel.disabled = false;
	this.controller.modelChanged(this.AvatartoggleModel);
	this.AliastoggleModel.disabled = false;
	this.controller.modelChanged(this.AliastoggleModel);
	this.BadCerttoggleModel.disabled = false;
	this.controller.modelChanged(this.BadCerttoggleModel);
	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.jabber")
	{
		this.jabberresourceModel.disabled = false;
		this.controller.modelChanged(this.jabberresourceModel);
	}

	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sametime")
	{
		this.SametimetoggleModel.disabled = false;
		this.controller.modelChanged(this.SametimetoggleModel);
	}

	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.sipe")
	{
		this.sipeloginModel.disabled = false;
		this.controller.modelChanged(this.sipeloginModel);
		this.sipeuseragentModel.disabled = false;
		this.controller.modelChanged(this.sipeuseragentModel);
		this.sipeproxytoggleModel.disabled = false;
		this.controller.modelChanged(this.sipeproxytoggleModel);
	}

	if (this.params.initialTemplate.templateId == "org.webosinternals.messaging.xfire")
	{
		this.xfireversionModel.disabled = false;
		this.controller.modelChanged(this.xfireversionModel);
	}
};

WizardAssistant.prototype.popScene = function() {
	if (this.params.aboutToActivateCallback !== undefined) {
		this.params.aboutToActivateCallback(true);
	}
	Mojo.Log.info("WizardAssistant popping scene.");
	Mojo.Log.info("WizardAssistant accountSettings=", JSON.stringify(this.accountSettings));
	this.controller.stageController.popScene(this.accountSettings);
};

WizardAssistant.prototype.LoadDonateWeb = function() {
	this.controller.serviceRequest('palm://com.palm.applicationManager', {
		method: 'open',
		parameters: {
			target: this.params.donationURL
		}
	}
)};

WizardAssistant.prototype.TextBoxNumbers = function(KeyCode) {
	//Only allow numbers
	if (KeyCode == 46 || (KeyCode > 47 && KeyCode < 58))
	{
		return true;
	}
	return false;
};
