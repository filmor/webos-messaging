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

        if (this.params && this.params.initialTemplate)
            this.template = this.params.initialTemplate;
        else
            this.template = {"loc_prpl": "prpl-icq"};

        var locale = enyo.g11n && enyo.g11n.toString();

        var call_params = { prpl: this.template.loc_prpl }

        if (enyo.g11n.toISOString)
            call_params["locale"] = enyo.g11n.toISOString();

        this.$.getOptions.call({ params: [ call_params ] });
    },

    gotOptions: function(inSender, inResponse) {
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
            templateId: "TEMPLATE",
            config: { prpl: "prpl-icq", preferences: this.prefs }
        }

        // TODO: Allow updating an account
        this.$.validateAccount.call({
            params: [params],
        });
    },

    validationSuccess: function(inSender, inResponse) {
        console.log(arguments);
        console.log("Success!");
        console.log(inResponse);
        this.$.crossAppResult.sendResult(inResponse);
    },

    validationFail: function() {
        console.log(arguments);
    },

    validationResponse: function() {
        this.$.username.setDisabled(false);
        this.$.password.setDisabled(false);
        this.$.options.setDisabled(false);

        this.$.createButton.setCaption(AccountsUtil.BUTTON_SIGN_IN);
        this.$.createButton.setActive(false);
        this.$.createButton.setDisabled(false);
    },

    components: [
        {
            kind: "Toolbar",
            className: "enyo-toolbar-light accounts-header",
            pack: "center",
            components: [
                { kind: "Image", name: "icon" },
                { kind: "Control", name: "title", content: "Create Account" }
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
                            caption: "USERNAME FROM TEMPLATE",
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
                            caption: $L("PASSWORD"),
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
            name: "getOptions",
            kind: "Purple.AccountService",
            onSuccess: "gotOptions",
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
