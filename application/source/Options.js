// Helper kind
//
// TODO: Handle enter key (blur?)
//
enyo.kind({
    name: "Purple.OptionsGroup",
    kind: "RowGroup",
    caption: $L("Advanced Options"),

    events: { onPreferenceChanged: "" },
    published: { disabled: false },

    disabledChanged: function() {
        // TODO
    },

    filterInt: function(inSender, inEvent) {
        var key = inEvent.keycode || inEvent.which;
        if (key == 13) // Enter
            console.log(inSender);

        key = String.fromCharCode(key);
        var regex = /[0-9]/;
        if (!regex.test(key)) {
            inEvent.preventDefault();
        }
    },

    handleIntInput: function(inSender) {
        var val = Number(inSender.getValue());

        if (val != NaN)
        {
            inSender.last_value = val;
            this.doPreferenceChanged(inSender.name, val);
        }
        else
        {
            inSender.setValue(inSender.last_value);
        }
    },

    handleChange: function(inSender) {
        this.doPreferenceChanged(inSender.name, inSender.getValue());
    },

    handleCheck: function(inSender) {
        this.doPreferenceChanged(inSender.name, inSender.getChecked());
    }
});

enyo.kind({
    name: "Purple.Options",
    kind: "Control",

    events: {
        onPreferenceChanged: ""
    },

    published: {
        options: {},
        disabled: false
    },

    optionsChanged: function() {
        if (this.$._group)
            this.$._group.destroy();

        this.createComponent({
            name: "_group",
            kind: "Purple.OptionsGroup",
            onPreferenceChanged: "doPreferenceChanged"
        })

        for (var name in this.options)
        {
            var node = this.options[name];

            var innerComponent = new Object();

            if (node.type == "string")
            {
                innerComponent = {
                    kind: "Input",
                    hint: node.type,
                    autocorrect: false,
                    spellcheck: false,
                    value: node.default_value,
                    oninput: "handleChange"
                }
            }
            else if (node.type == "int")
            {
                innerComponent = {
                    kind: "Input",
                    hint: node.type,
                    autocorrect: false,
                    spellcheck: false,
                    value: node.default_value,
                    last_value: node.default_value,
                    oninput: "handleIntInput",
                    onkeypress: "filterInt"
                };
            }
            else if (node.type == "bool")
            {
                innerComponent = {
                    kind: "CheckBox",
                    checked: node.default_value,
                    onChange: "handleCheck"
                };
            }
            else if (node.type == "list")
            {
                var items = [];
                for (var name_ in node.choices)
                    items.push({caption: node.choices[name_],
                                value: name_
                                });

                innerComponent = {
                    kind: "ListSelector",
                    items: items,
                    onChange: "handleChange"
                };
            }
            else
                continue;

            innerComponent["name"] = name;

            this.$._group.createComponent({
                layoutKind: "HFlexLayout",
                components: [
                    {content: node.text, flex: 1},
                    innerComponent
                ]
            });
        }
    },

    disabledChanged: function() {
        if (this.$._group)
            this.$._group.setDisabled(this.disabled);
    },

})
