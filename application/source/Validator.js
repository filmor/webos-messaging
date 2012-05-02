enyo.kind({
    name: "Purple.AccountService",
    kind: "PalmService",
    service: "palm://org.webosinternals.purple.validator/"
});

enyo.kind({
    name: "Purple.Validator",
    kind: "VFlexBox",
    className: "basic-back",
    width: "100%",
    prefs: {},

    create: function(params) {
        this.inherited(arguments);

        if (params) 
        {
            this.template = params.initialTemplate || params.template;
            this.prefs = this.template.loc_preferences || {};
        }
        else
        {
            this.template = {"templateId": "org.webosinternals.purple.icq",
                             "loc_prpl": "prpl-icq"};
        }
        
        if ("loc_usernameLabel" in this.template)
            this.$.usernameGroup.caption = this.template.loc_usernameLabel;

        if ("loc_passwordLabel" in this.template)
            this.$.passwordGroup.caption = this.template.loc_passwordLabel;

        enyo.log(this.template.loc_prpl);

        var locale = enyo.g11n && enyo.g11n.toString();

        var call_params = { prpl: this.template.loc_prpl }

        if (enyo.g11n.toISOString)
            call_params["locale"] = enyo.g11n.toISOString();

        this.$.getOptions.call({ params: [ call_params ] });
    },

    gotOptions: function(inSender, inResponse) {
        enyo.log(inResponse);
        this.$.options.setOptions(inResponse.options);
        this.render();
    },

    prefChanged: function(inSender, key, value) {
        this.prefs[key] = value;
    },

    signIn: function() {
        this.$.username.setDisabled(true);
        this.$.password.setDisabled(true);
        this.$.options.setDisabled(true);
        
        this.$.createButton.setCaption(AccountsUtil.BUTTON_SIGNING_IN);
        this.$.createButton.setActive(true);
        this.$.createButton.setDisabled(true);

        var params = {
            username: this.$.username.getValue(),
            password: this.$.password.getValue(),
            templateId: this.template.templateId,
            config: { prpl: this.template.loc_prpl, preferences: this.prefs }
        }

        this.createComponent({
            kind: "Purple.AccountService",
            name: "getUIEvents",
            onResponse: "uiEvent",
            subscribe: true
        });
        this.$.getUIEvents.call();

        // TODO: Allow updating an account
        this.$.validateAccount.call({
            params: [params],
        });
    },

    validationSuccess: function(inSender, inResponse) {
        this.$.crossAppResult.sendResult(inResponse);
    },

    validationFail: function() {
        enyo.log(arguments);
    },

    validationResponse: function() {
        this.$.username.setDisabled(false);
        this.$.password.setDisabled(false);
        this.$.options.setDisabled(false);

        this.$.getUIEvents.destroy();

        this.$.createButton.setCaption(AccountsUtil.BUTTON_SIGN_IN);
        this.$.createButton.setActive(false);
        this.$.createButton.setDisabled(false);
    },

    uiEvent: function(inSender, inResponse) {
        this.$.popupGenerator.showPopup(inResponse);
    },

    popupAction: function(inSender, inResponse) {
        enyo.log(inResponse);
        if ('answer' in inResponse && 'id' in inResponse) {
            this.$.answerUIEvent.call({params: inResponse});
        }
        else
        {
            enyo.log("Something is wrong here", inResponse);
        }
    },

    components: [
        {
            kind: "Toolbar",
            className: "enyo-toolbar-light accounts-header",
            pack: "center",
            components: [
                {
                    kind: "Image",
                    name: "icon"
                },
                {
                    kind: "Control",
                    name: "title",
                    content: "Create Account"
                }
            ]
        },
        { className: "accounts-header-shadow" },
        {
            name: "validator",
            kind: "Scroller",
            flex: 1,
            components: [
                {
                    className: "box-center",
                    components: [
                        {
                            kind: "RowGroup",
                            name: "usernameGroup",
                            caption: AccountsUtil.LIST_TITLE_USERNAME,
                            className: "accounts-group",
                            components: [
                                {
                                    name: "username",
                                    kind: "Input",
                                    inputType: "email",
                                    value: "",
                                    spellcheck: false,
                                    autoCapitalize: "lowercase",
                                    autocorrect: false,
                                    oninput: "validateInput",
                                    onkeydown: "checkForEnter"
                                }
                            ]
                        },
                        {
                            kind: "RowGroup",
                            name: "passwordGroup",
                            caption: AccountsUtil.LIST_TITLE_PASSWORD,
                            className: "accounts-group",
                            components: [
                                {
                                    name: "password",
                                    kind: "Input",
                                    inputType: "password",
                                    value: "",
                                    spellcheck: false,
                                    autocorrect: false,
                                    oninput: "validateInput",
                                    onkeydown: "checkForEnter"
                                }
                            ]
                        },
                        {
                            kind: "Purple.Options",
                            name: "options",
                            onPreferenceChanged: "prefChanged"
                        },
                        {
                            name: "createButton",
                            kind: "ActivityButton",
                            disabled: true,
                            active: false,
                            caption: AccountsUtil.BUTTON_SIGN_IN,
                            className: "enyo-button-dark accounts-btn",
                            onclick: "signIn"
                        }
                    ]
                }
            ]
        },
        {
            name: "popupGenerator",
            kind: "Purple.PopupGenerator",
            onAction: "popupAction"
        },
        {
            name: "getOptions",
            kind: "Purple.AccountService",
            onSuccess: "gotOptions",
        },
        {
            name: "answerUIEvent",
            kind: "Purple.AccountService"
        },
        {
            name: "validateAccount",
            kind: "Purple.AccountService",
            onSuccess: "validationSuccess",
            onFailure: "validationFail",
            onResponse: "validationResponse"
        },
        { kind: "CrossAppResult" }
    ],

    validateInput: function() {
        this.$.createButton.setDisabled(
            this.$.username.isEmpty() || this.$.password.isEmpty()
        );
    },

    checkForEnter: function(inSender, inResponse) {
        if (inResponse.keyCode != 13) {
            return;
        }

        if (inSender.getName() === "username") {
            enyo.asyncMethod(this.$.password, "forceFocus");
        }
        else {
            if (!this.$.createButton.getDisabled()) {
                this.$.password.forceBlur();
                this.signIn();
            }
            else {
                enyo.asyncMethod(this.$.username, "forceFocus");
            }
        }
    }
});
