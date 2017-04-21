$( document ).ready(function() {
    
    $('#numberOfCounter').on('focusout', function() {
        var numberOfCounter = $(this).val();
        
        $('.inputDetails').remove();
        $('#inputForm').append('<div class="inputDetails"></div>');

        for (var i = 1; i <= numberOfCounter; i++) {
            var numberOfEmployeeInput = `<div class="form-input">` +
                                            `<label for="numberOfEmployee${i}">Number of Employee in Counter #${i} </label>` +
                                            `<input type="text" name="numberofEmployee[]">` +
                                        `</div>`

            $('.inputDetails').append(numberOfEmployeeInput);
        }
    });

    
});